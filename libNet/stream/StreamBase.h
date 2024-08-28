#pragma once
//========================================================================
//[File Name]:StreamBase.h
//[Description]: these file contains the basic definitions of tcp Server, connector,
//  client,CaSSL.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include <functional>
#include <list>
#include <iostream>
#include <thread>
#include <atomic>

#include "../core/SessionBase.h"
#include "Event.h"
#include "SessionIdGen.h"


namespace Net {
	using namespace Core;

	namespace Stream {
		template<typename SocketType>
		class NetBase : public std::enable_shared_from_this<NetBase<SocketType>> {
		public:
			explicit NetBase(io_context& io_c, unsigned timeout) :ioCtx_(io_c), \
				memStorage_({ 32, 64, 256, 512, 1024 }), timer_(io_c), timeout_(timeout) { }
			virtual ~NetBase() = default;

			int Close(int session_id) {
				this->CloseSession(session_id);
				StreamWriter::Instance()->Write(std::cout, "[NetBase] disconnect ok! session_id=%d", session_id);
				return 0;
			}
			int IsConnected(int session_id) const {
				auto find = this->umSessions_.find(session_id);
				if (find != this->umSessions_.end())
					return find->second->GetStatus();
				return -1;
			}
			int Send(int session_id, const char* data, unsigned len, uint8_t& err_code) {
				auto find = this->umSessions_.find(session_id);
				if (find != this->umSessions_.end()) {
					if (!find->second->GetStatus())
						return -1;
					else
						return find->second->Send(data, len, err_code);
				}
				return -2;
			}
			int AsyncSend(int session_id, const char* data, unsigned len, CBAsyncSend callback = nullptr) {
				auto find = this->umSessions_.find(session_id);
				if (find != this->umSessions_.end()) {
					if (!find->second->GetStatus())
						return -1;
					else {
						return find->second->AsyncSend(data, len, [this, session_id, callback](size_t byte_send, uint8_t ec) {
							if (callback)
								callback(session_id, byte_send, ec);
							});
					}
				}
				return -2;
			}
			int Recieve(int session_id, char** data, uint8_t& err_code) {
				auto find = this->umSessions_.find(session_id);
				if (find != this->umSessions_.end()) {
					if (!find->second->GetStatus())
						return -1;
					else
						return find->second->Recieve(data, err_code);
				}
				return -2;
			}
			int AsyncRecieve(int session_id, CBAsyncRecv callback = nullptr) {
				auto find = this->umSessions_.find(session_id);
				if (find != this->umSessions_.end()) {
					if (!find->second->GetStatus())
						return -1;
					else
						return find->second->AsyncRecieve([this, callback, session_id](Packet* packet, uint8_t ec) {
							if (packet == nullptr) {
								StreamWriter::Instance()->Write(std::cout, "[NetBase] Async recieve err, code=%d", (int)ec);
								if (callback)
									callback(session_id, nullptr, 0, ec);
							}
							else {
								if (callback)
									callback(session_id, packet->body(), packet->bodyLen(), ec);
							}
						});
				}
				return -2;
			}

			template<typename F>
			void SetEventCB(F f) {
				cbEvent_ = f;
			}
		protected:
			int CloseAllSession() {
				try
				{
					std::lock_guard<std::mutex> lock(mtx_);
					for (auto it = umSessions_.begin(); it != umSessions_.end();) {
						it->second->Close();
						SessionIdGen<int>::Instance()->ReleaseId(it->first);
						it = umSessions_.erase(it);

						StreamWriter::Instance()->Write(std::cout, "[NetBase] Current connect count=%d", umSessions_.size());
					}
					return 0;
				}
				catch (const std::exception& e)
				{
					StreamWriter::Instance()->Write(std::cout, "[NetBase]:Close cnn raise ex:%s", e.what());
					throw e;
					return -1;
				}
			}
			int CloseSession(int session_id) {
				std::lock_guard<std::mutex> lock(mtx_);
				if (umSessions_.find(session_id) != umSessions_.end()) {
					try
					{
						umSessions_.at(session_id)->Close();
						umSessions_.erase(session_id);
						SessionIdGen<int>::Instance()->ReleaseId(session_id);

						Event event;
						event.type = Event::eEvent_t::e_connClosed;
						if (cbEvent_)
							cbEvent_(session_id, event);

						StreamWriter::Instance()->Write(std::cout, "[NetBase] Current connect count=%d", umSessions_.size());
						return 0;
					}
					catch (const std::exception& e)
					{
						StreamWriter::Instance()->Write(std::cout, "[NetBase]:Close cnn raise ex:%s", e.what());
						throw e; return -1;
					}
				}
				return 0;
			}

			virtual void HandleRWErr(int session_id) = 0;

			virtual void OnTimer(SOCKET_TYPE& socket) = 0;
			void OffTimer() {
				if (this->timeout_ > 0)
					timer_.cancel();
			}

		protected:
			bool bExit_{ false };
			bool bAsyncAc_{ false };
			io_context& ioCtx_;
			steady_timer timer_;
			unsigned timeout_{ 0 };
			//std::unordered_map<int, Session*> umSessions_;
			std::unordered_map<int, std::shared_ptr<SessionBase<SocketType>>> umSessions_;
			NMemoryStorage<char>	memStorage_;
			CBEvent	  cbEvent_{ nullptr };
			std::mutex mtx_;
		};

		using CBReceive = std::function<void(int, const char*, size_t, uint8_t)>;
		using CBSend = std::function<void(int, size_t, uint8_t)>;

		class ServerBase : public NetBase<SOCKET_TYPE>, public std::enable_shared_from_this<ServerBase> {
		public:
			explicit ServerBase(io_context& io_c, const std::string& ip, short port, unsigned timeout) : NetBase<SOCKET_TYPE>(io_c, timeout), \
				acceptor_(io_c) {
				tcp::endpoint end_point(asio::ip::make_address(ip), port);
				acceptor_.open(end_point.protocol());
				acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true));
				acceptor_.bind(end_point);
				acceptor_.listen();
			}

			void Start() {
				this->bAsyncAc_ = false;
				this->bExit_ = false;
				StreamWriter::Instance()->Write(std::cout, "[ServerBase] Start listening!");
				Accept();
			}
			void StartA() {
				try
				{
					this->bAsyncAc_ = true;
					this->bExit_ = false;
					StreamWriter::Instance()->Write(std::cout, "[ServerBase] StartA listening!");

					this->AsyncAccept();
					std::this_thread::sleep_for(std::chrono::milliseconds(5));
				}
				catch (const std::exception& e)
				{
					StreamWriter::Instance()->Write(std::cout, "[ServerBase] StartA raise ex:%s", e.what());
					throw e;//todo
				}
			}
			void Stop() {
				this->CloseAllSession();
				this->bExit_ = true;

				for (auto it : wkThds_) {
					if (it && it->joinable()) {
						it->join(); it = nullptr;
					}
				}
				StopAccept();
				if (ServThdptr_ && ServThdptr_->joinable()) {
					ServThdptr_->join(); ServThdptr_ = nullptr;
				}
				StreamWriter::Instance()->Write(std::cout, "[ServerBase] Stop!");
			}
			template<typename F>
			void SetReceiveCB(F f) { cbReceive_ = f; }

		protected:
			virtual void Accept() = 0;
			virtual void AsyncAccept() = 0;

			void DoReceive(int id) {
				std::shared_ptr<std::thread> work_thread = std::make_shared<std::thread>([this, id] {
					while (!this->bExit_) {
						try
						{
							uint8_t err_code{ 0 };
							char* data{ nullptr };
							auto size = this->Recieve(id, &data, err_code);
							if (size < 0) {
								if (err_code == 1 || err_code == 2) { //todo
									StreamWriter::Instance()->Write(std::cout, "[ServerBase] Recieve with err=%d", (int)err_code);
									break;
								}
								else {
									//todo to process the packet error
									StreamWriter::Instance()->Write(std::cout, "[ServerBase] Recieve with err=%d", (int)err_code);
								}
							}

							static int num = 0;
							auto thd_id = std::this_thread::get_id();
							StreamWriter::Instance()->Write(std::cout, "[ServerBase] thead id=%d, Conn Num=%d, session id=%d£¬packet count=%d, Packet:%s", \
								thd_id, this->umSessions_.size(), id, num++, data);

							if (cbReceive_)
								cbReceive_(id, data, size, err_code);
						}
						catch (const std::exception& e)
						{
							StreamWriter::Instance()->Write(std::cout, "[ServerBase] Recieve raise ex:%s", e.what());
							throw e; break;
						}
					}
				});
				this->wkThds_.emplace_back(work_thread);
			}
			void DoAsyncReceive(int id) {
				this->AsyncRecieve(id, [this](int id, char* data, unsigned len, uint8_t ec) {
					if (data) {
						static int num = 0;
						auto thd_id = std::this_thread::get_id();
						std::string str_packet = std::string(data, len);
						StreamWriter::Instance()->Write(std::cout, "[ServerBase] thead id=%d, Conn Num=%d, session id=%d£¬packet count=%d, Packet:%s", \
							thd_id, this->umSessions_.size(), id, num++, str_packet.c_str());

						if (cbReceive_)
							cbReceive_(id, data, len, ec);
					}
					if (ec) {
						//todo to process the packet error
						StreamWriter::Instance()->Write(std::cout, "[ServerBase] Async recieve with err =%d", (int)ec);
					}
					this->DoAsyncReceive(id);
				});
			}
			void HandleAcpErr(const asio::error_code& ec) {
				StreamWriter::Instance()->Write(std::cout, "[ServerBase] Err, reason:=%s", ec.message().c_str());
				StopAccept();
			}
			virtual void HandleRWErr(int session_id)  override {
				StreamWriter::Instance()->Write(std::cout, "[ServerBase] RW err, session id=%d", session_id);
				this->CloseSession(session_id);
			}
			void StopAccept() {
				asio::error_code ec;
				acceptor_.cancel(ec);
				acceptor_.close(ec);
				this->ioCtx_.stop();
			}

		protected:
			tcp::acceptor acceptor_;
			std::shared_ptr<std::thread> ServThdptr_{ nullptr };
			std::list<std::shared_ptr<std::thread>> wkThds_{ nullptr };
			CBReceive cbReceive_{ nullptr };
		};

		class ConnectorBase : public NetBase<SOCKET_TYPE>, public std::enable_shared_from_this<ConnectorBase> {
		public:
			explicit ConnectorBase(io_context& io_c, const std::string& ip, short port, unsigned timeout) : NetBase<SOCKET_TYPE>(io_c, timeout),
				ip_(ip), port_(port) { CheckConnect(); }
			virtual ~ConnectorBase() {
				this->CloseAllSession();
				this->bExit_ = true;

				if (reconnThread_ && reconnThread_->joinable()) {
					reconnThread_->join();
					reconnThread_ = nullptr;
				}
			}

			virtual int Open(const std::string& ip, short port, bool keep_live = false) = 0;
			virtual void OpenA(const std::string& ip, short port, std::function<void(int, std::error_code)> func, bool keep_live = false) = 0;

		protected:
			void CheckConnect() {
				if (reconnThread_ != nullptr)
					return;

				reconnThread_ = std::make_shared<std::thread>([this] {
					try
					{
						while (!this->bExit_) {
							{
								std::lock_guard<std::mutex> lock(this->mtx_);
								if (!this->umSessions_.empty()) {
									for (auto it : this->umSessions_) {
										if (it.second->GetKeeplive() && !it.second->GetStatus()) {
											//async reconnect
											ReConn(it.first);
										}
										int tm = ((float)this->timeout_ / this->umSessions_.size()) * 1000;
										std::this_thread::sleep_for(std::chrono::milliseconds(tm > 10 ? tm : 10));
									}
								}
							}
							std::this_thread::sleep_for(std::chrono::seconds(1));
						}
					}
					catch (const std::exception& e)
					{
						StreamWriter::Instance()->Write(std::cout, "[ConnectorBase] reconnect raise ex, reason=%s", e.what());
						throw e;
					}
					});
			}
			void HandleConnErr(int session_id, const asio::error_code& ec) {
				StreamWriter::Instance()->Write(std::cout, "[ConnectorBase] Conn err, session_id=%d, reasion:%s", session_id, ec.message().c_str());
				auto find = this->umSessions_.find(session_id);
				if (find != this->umSessions_.end() && find->second->GetKeeplive())
					CheckConnect();
				else
					this->CloseSession(session_id);
			}
			virtual void HandleRWErr(int session_id)  override {
				StreamWriter::Instance()->Write(std::cout, "[ConnectorBase] RW err, session id=%d", session_id);
				auto find = this->umSessions_.find(session_id);
				if (find != this->umSessions_.end() && find->second->GetKeeplive())
					CheckConnect();
				else
					this->CloseSession(session_id);
			}
			virtual void ReConn(int session_id) = 0;

		protected:
			std::string ip_;
			short port_{ 0 };
			std::shared_ptr<std::thread> reconnThread_{ nullptr };
			std::atomic_bool bAsyncW_{ false };
		};

		template<typename SOCKET_TYPE>
		class ClientBase : public std::enable_shared_from_this<ClientBase<SOCKET_TYPE>> {
		public:
			explicit ClientBase(io_context& io_c, const std::string& ip, short port, unsigned timeout) : ioCtx_(io_c),
				timeout_(timeout), timer_(io_c), ip_(ip), port_(port), memStorage_({ 32, 64, 256, 512, 1024 })
			{ }
			~ClientBase() {
				SAFE_DELETE(session_);
			}

			virtual int Open(const std::string& ip, short port, bool keep_live = false) = 0;
			virtual void OpenA(const std::string& ip, short port, \
				std::function<void(int, std::error_code)> func, bool keep_live = false) = 0;

			int Close() {
				CloseSession();
				StreamWriter::Instance()->Write(std::cout, "[ClientBase] disconnect ok!");
				return 0;
			}
			int IsConnected() const {
				return session_->GetStatus();
			}
			int Send(const char* data, unsigned len, uint8_t& err_code) {
				if (!session_->GetStatus())
					return -1;
				else
					return session_->Send(data, len, err_code);
			}
			int AsyncSend(const char* data, unsigned len, CBAsyncSend cb = nullptr) {
				if (!session_->GetStatus())
					return -1;
				else
					return session_->AsyncSend(data, len, cb);
			}
			int Recieve(char** data, uint8_t& err_code) {
				if (!session_->GetStatus())
					return -1;
				else
					return session_->Recieve(data, err_code);
			}
			int AsyncRecieve(int session_id, CBAsyncRecv cb = nullptr) {
				if (!session_->GetStatus())
					return -1;
				else
					return session_->AsyncRecieve([cb](Packet* packet, uint8_t ec) {
						if (packet == nullptr) {
							StreamWriter::Instance()->Write(std::cout, "[ClientBase] Async recieve err, code=%d", (int)ec);
							if (cb)
								cb(nullptr, 0, ec);
						}
						else {
							if (cb)
								cb(packet->body(), packet->bodyLen(), ec);
						}
					});
			}

			template<typename F>
			void SetEventCB(F f) { cbEvent_ = f; }

		protected:
			int CloseSession() {
				try
				{
					int id = session_->GetId();
					session_->Close();
					SessionIdGen<int>::Instance()->ReleaseId(id);
					delete session_;

					Event event;
					event.type = Event::eEvent_t::e_connClosed;
					if (cbEvent_)
						cbEvent_(id, event);

					StreamWriter::Instance()->Write(std::cout, "[ClientBase] Close success.");
					return 0;
				}
				catch (const std::exception& e)
				{
					StreamWriter::Instance()->Write(std::cout, "[ClientBase] Close raise ex:%s", e.what());
					throw e; return -1;
				}
				return 0;
			}
			void CheckConnect() {
				if (reconnThread_ != nullptr)
					return;

				reconnThread_ = std::make_shared<std::thread>([this] {
					try
					{
						while (!this->bExit_) {
							if (this->session_->GetKeeplive() && !this->session_->GetStatus()) {
								//async reconnect
								ReConn();
							}
							std::this_thread::sleep_for(std::chrono::seconds(this->timeout_));
						}
					}
					catch (const std::exception& e)
					{
						StreamWriter::Instance()->Write(std::cout, "[ClientBase] reconnect raise ex, reason=%s", e.what());
						throw e;
					}
				});
			}

			virtual void ReConn() = 0;
			void HandleConnErr(const asio::error_code& ec) {
				std::cout << "[ClientBase] Conn Err, reasion : " << ec.message() << std::endl;
				CheckConnect();
			}
			void HandleRWErr(int conn_id) {
				std::cout << "[ClientBase] RW err, connect id : " << conn_id << std::endl;
				CheckConnect();
			}

			virtual void OnTimer(SOCKET_TYPE& socket) = 0;
			void OffTimer() {
				if (this->timeout_ > 0)
					timer_.cancel();
			}

		protected:
			io_context& ioCtx_;
			std::string ip_;
			short port_{ 0 };
			SessionBase<SOCKET_TYPE>* session_{ nullptr };
			steady_timer timer_;
			unsigned timeout_{ 0 };
			NMemoryStorage<char> memStorage_;
			CBEvent	cbEvent_{ nullptr };
			bool bExit_{ false };
			std::shared_ptr<std::thread> reconnThread_{ nullptr };
		};

#ifdef OPENSSL
		class CaSSL {
		public:
			explicit CaSSL(const std::string& ca): sslCtx_(ssl::context::sslv23) {
				try
				{
					sslCtx_.load_verify_file(ca);
				}
				catch (const std::exception& e)
				{
					StreamWriter::Instance()->Write(std::cout, "[CaSSL] load ca file:%s, raise ex=%s", ca.c_str(), e.what());
					throw e;
				}
			}

		protected:
			void SetVerify(SOCKET_TYPE& socket_ssl) {
				socket_ssl.set_verify_mode(ssl::verify_peer);
				socket_ssl.set_verify_callback(std::bind(&CaSSL::VerifyCertificate, \
					this, std::placeholders::_1, std::placeholders::_2));
			}
			bool VerifyCertificate(bool preverified, ssl::verify_context& ctx) {
				char subject_name[256]{ 0 };
				X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
				X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);
				StreamWriter::Instance()->Write(std::cout, "[CaSSL] Verifying=%s", subject_name);

				return preverified;
			}

		protected:
			ssl::context sslCtx_;
		};
#endif //OPENSSL
	}
}