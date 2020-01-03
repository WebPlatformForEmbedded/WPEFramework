#ifndef __COM_MESSAGES_H
#define __COM_MESSAGES_H

#include "Module.h"

namespace WPEFramework {
namespace RPC {
    #define COMRPC_POINTER_LENGTH 32

    #if COMRPC_POINTER_LENGTH == 32
        typedef uint32_t instanceId_t;
    #elif COMRPC_POINTER_LENGTH == 64
        typedef uint64_t instanceId_t;
        // 128bit cpus might not be kings of the market, but this definition is usefull for debugging :)
    #elif COMRPC_POINTER_LENGTH == 128 
        typedef __uint128_t instanceId_t; 
    #else
        #warning "COMRPC_POINTER_LENGTH set to unsupported value. Using 32 as default!"
        typedef uint32_t instanceId_t;
    #endif

    constexpr instanceId_t EmptyInstance = 0;

    namespace Data {
        static const uint16_t IPC_BLOCK_SIZE = 512;

        class Frame : public Core::FrameType<IPC_BLOCK_SIZE> {
        private:
            Frame(Frame&) = delete;
            Frame& operator=(const Frame&) = delete;

        public:
            Frame()
            {
            }
            ~Frame()
            {
            }

        public:
            friend class Input;
            friend class Output;
            friend class ObjectInterface;

            uint16_t Serialize(const uint16_t offset, uint8_t stream[], const uint16_t maxLength) const
            {
                uint16_t copiedBytes((Size() - offset) > maxLength ? maxLength : (Size() - offset));

                ::memcpy(stream, &(operator[](offset)), copiedBytes);

                return (copiedBytes);
            }
            uint16_t Deserialize(const uint16_t offset, const uint8_t stream[], const uint16_t maxLength)
            {
                Size(offset + maxLength);

                ::memcpy(&(operator[](offset)), stream, maxLength);

                return (maxLength);
            }
        };

        class Input {
        private:
            Input(const Input&) = delete;
            Input& operator=(const Input&) = delete;

        public:
            Input()
                : _data()
            {
            }
            ~Input()
            {
            }

        public:
            inline void Clear()
            {
                _data.Clear();
            }
            void Set(instanceId_t instanceId, const uint32_t interfaceId, const uint8_t methodId)
            {
                uint16_t result = _data.SetNumber<instanceId_t>(0, instanceId);
                result += _data.SetNumber<uint32_t>(result, interfaceId);
                _data.SetNumber(result, methodId);
            }
            instanceId_t InstanceId()
            {
                instanceId_t instanceId = RPC::EmptyInstance;

                _data.GetNumber<instanceId_t>(0, instanceId);

                return instanceId;
            }
            uint32_t InterfaceId() const
            {
                uint32_t result = 0;

                _data.GetNumber<uint32_t>(sizeof(instanceId_t), result);

                return (result);
            }
            uint8_t MethodId() const
            {
                uint8_t result = 0;

                _data.GetNumber(sizeof(instanceId_t) + sizeof(uint32_t), result);

                return (result);
            }
            uint32_t Length() const
            {
                return (_data.Size());
            }
            inline Frame::Writer Writer()
            {
                return (Frame::Writer(_data, (sizeof(instanceId_t) + sizeof(uint32_t) + sizeof(uint8_t))));
            }
            inline const Frame::Reader Reader() const
            {
                return (Frame::Reader(_data, (sizeof(instanceId_t) + sizeof(uint32_t) + sizeof(uint8_t))));
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_data.Serialize(static_cast<uint16_t>(offset), stream, maxLength));
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_data.Deserialize(static_cast<uint16_t>(offset), stream, maxLength));
            }

        private:
            Frame _data;
        };

        class Output {
        private:
            Output(const Output&) = delete;
            Output& operator=(const Output&) = delete;

        public:
            Output()
                : _data()
            {
            }
            ~Output()
            {
            }

        public:
            inline void Clear()
            {
                _data.Clear();
            }
            inline Frame::Writer Writer()
            {
                return (Frame::Writer(_data, 0));
            }
            inline const Frame::Reader Reader() const
            {
                return (Frame::Reader(_data, 0));
            }
            inline void AddInstanceId(instanceId_t instanceId, const uint32_t id)
            {
                _data.SetNumber<instanceId_t>(_data.Size(), instanceId);
                _data.SetNumber<uint32_t>(_data.Size(), id);
            }
            inline uint32_t Length() const
            {
                return (static_cast<uint32_t>(_data.Size()));
            }
            inline uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_data.Serialize(static_cast<uint16_t>(offset), stream, maxLength));
            }
            inline uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_data.Deserialize(static_cast<uint16_t>(offset), stream, maxLength));
            }

        private:
            Frame _data;
        };

        class Init {
        private:
            Init(const Init&) = delete;
            Init& operator=(const Init&) = delete;
            uint32_t ParentId() const 
            {
                uint32_t exchangeId = 0;
                string value; Core::SystemInfo::GetEnvironment(_T("COM_PARENT_EXCHANGE_ID"), value);
                if (value.empty() == false) {
                    exchangeId = Core::NumberType<uint32_t>(value.c_str(), static_cast<uint32_t>(value.length())).Value();
                }

                return (exchangeId);
            }

        public:
            enum type : uint8_t {
                AQUIRE = 0,
                OFFER = 1,
                REVOKE = 2,
                REQUEST = 3
            };

        

        public:
            Init()
                : _id(0)
				, _instanceId(RPC::EmptyInstance)
                , _interfaceId(~0)
                , _exchangeId(~0)
                , _versionId(0)
            {
            }
            ~Init()
            {
            }

        public:
            bool IsOffer() const
            {
                return (_className[0] == '\0') && (_className[1] == OFFER);
            }
            bool IsRevoke() const
            {
                return (_className[0] == '\0') && (_className[1] == REVOKE);
            }
            bool IsRequested() const
            {
                return (_className[0] == '\0') && (_className[1] == REQUEST);
            }
            bool IsAquire() const
            {
                return (IsRevoke() == false) && (IsOffer() == false) && (IsRequested() == false);
            }
            void Set(const uint32_t myId)
            {
                _exchangeId = ParentId();
                _instanceId = RPC::EmptyInstance;
                _interfaceId = ~0;
                _versionId = ~0;
                _id = myId;
                _className[0] = '\0';
                _className[1] = AQUIRE;
   
            }
            void Set(const uint32_t myId, const uint32_t interfaceId, instanceId_t instanceId, const uint32_t exchangeId)
            {
                _exchangeId = exchangeId;
                _instanceId = instanceId;
                _interfaceId = interfaceId;
                _versionId = 0;
                _id = myId;
                _className[0] = '\0';
                _className[1] = REQUEST;
            }
            void Set(const uint32_t myId, const uint32_t interfaceId, instanceId_t instanceId, const type whatKind)
            {
                ASSERT((whatKind != AQUIRE) && (whatKind != REQUEST));

                _exchangeId = ParentId();
                _instanceId = instanceId;
                _interfaceId = interfaceId;
                _versionId = 0;
                _id = myId;
                _className[0] = '\0';
                _className[1] = whatKind;
            }
            void Set(const uint32_t myId, const string& className, const uint32_t interfaceId, const uint32_t versionId)
            {
                _exchangeId = ParentId();
                _instanceId = RPC::EmptyInstance;
                _interfaceId = interfaceId;
                _versionId = versionId;
                _id = myId;
                const std::string converted(Core::ToString(className));
                ::strncpy(_className, converted.c_str(), sizeof(_className));
            }
            uint32_t Id() const
            {
                return (_id);
            }
            instanceId_t InstanceId() const
            {
                return (_instanceId);
            }
            uint32_t InterfaceId() const
            {
                return (_interfaceId);
            }
            uint32_t ExchangeId() const
            {
                return (_exchangeId);
            }
            uint32_t VersionId() const
            {
                return (_versionId);
            }
            const string ClassName() const
            {
                return (Core::ToString(std::string(_className)));
            }

        private:
            uint32_t _id;
            instanceId_t _instanceId;
            uint32_t _interfaceId;
            uint32_t _exchangeId;
            uint32_t _versionId;
            char _className[64];
        };

        class Setup {
        private:
            Setup(const Setup&) = delete;
            Setup& operator=(const Setup&) = delete;

        public:
            Setup()
                : _data()
            {
            }
            ~Setup()
            {
            }

        public:
            inline void Clear()
            {
                _data.Clear();
            }
            void Set(instanceId_t instanceId, const string& proxyStubPath, const string& traceCategories)
            {
                _data.SetNumber<instanceId_t>(0, instanceId);
                uint16_t length = _data.SetText(sizeof(instanceId_t), proxyStubPath);
                _data.SetText(sizeof(instanceId_t) + length, traceCategories);
            }
			inline bool IsSet() const {
                return (_data.Size() > 0);
			}
            string ProxyStubPath() const
            {
                string value;

                _data.GetText(sizeof(instanceId_t), value);

                return (value);
            }
            string TraceCategories() const
            {
                string value;

                _data.GetText(sizeof(instanceId_t) + _data.GetText(sizeof(instanceId_t), value), value);

                return (value);
            }
            instanceId_t InstanceId() const
            {
                instanceId_t instanceId = RPC::EmptyInstance;
                _data.GetNumber<instanceId_t>(0, instanceId);
                return (instanceId);
            }
            void InstanceId(instanceId_t instanceId)
            {
                _data.SetNumber<instanceId_t>(0, instanceId);
            }
            uint32_t Length() const
            {
                return (_data.Size());
            }
            uint16_t Serialize(uint8_t stream[], const uint16_t maxLength, const uint32_t offset) const
            {
                return (_data.Serialize(static_cast<uint16_t>(offset), stream, maxLength));
            }
            uint16_t Deserialize(const uint8_t stream[], const uint16_t maxLength, const uint32_t offset)
            {
                return (_data.Deserialize(static_cast<uint16_t>(offset), stream, maxLength));
            }

        private:
            Frame _data;
        };
    }

    typedef Core::IPCMessageType<1, Data::Init, Data::Setup> AnnounceMessage;
    typedef Core::IPCMessageType<2, Data::Input, Data::Output> InvokeMessage;
}
}

#endif // __COM_MESSAGES_H
