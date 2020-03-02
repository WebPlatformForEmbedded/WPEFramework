#pragma once

#include <vector>
#include <string>
#include <array>

//TODO cplusplus
extern "C"
{
#include <stddef.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include <gbm.h>

#include <drm_fourcc.h>
}

class ModeSet final
{
    public :

        enum class TYPE : uint16_t {INITIAL = 0, CURRENT = 1};

        // enum, DRM_NODE_PRIMARY, DRM_NODE_RENDER, DRM_NODE_CONTROL

        ModeSet()   = delete;
        ModeSet(TYPE type) : _type_(type), _fd_(-1), _crtc_(0), _encoder_(0), _connector_(0), _mode_(0), _buffer_(nullptr)
        {}

        ModeSet(const ModeSet& rhs)
        {
// TODO: copy the type?

            _fd_ = rhs.fd();
            _crtc_ = rhs.crtc();
            _encoder_ = rhs.encoder();
            _connector_ = rhs.connector();
            _fb_ = const_cast<ModeSet&>(rhs).fb();
            _mode_ = const_cast<ModeSet&>(rhs).mode();

            // Provide access to the underlying buffer subsystem
            InitializeScanOutBuffer();
        }

        ModeSet& operator=(const ModeSet& rhs)
        {
            if(this != &rhs)
            {
// TODO: copy the type?

                _fd_ = rhs.fd();
                _crtc_ = rhs.crtc();
                _encoder_ = rhs.encoder();
                _connector_ = rhs.connector();

                // The rhs will not be modified. Promised!
                _fb_ = const_cast<ModeSet&>(rhs).fb();
                _mode_ = const_cast<ModeSet&>(rhs).mode();

                // Provide access to the underlying buffer subsystem
                InitializeScanOutBuffer();
            }

            return *this;
        }

        ~ModeSet()
        {
            /* bool */ destroy();
        }

        bool create();
        bool destroy();

        bool valid()
        {
            // _buffer_ is not considered part of the test
// TODO: consider fb being part of the test, eg 0 < fb
            return (0<= _fd_) && (0 <= _crtc_) && (0 <= _encoder_) && (0 <= _connector_);
        }

        ModeSet::TYPE type() const
        {
            return _type_;
        }

        uint32_t fd() const
        {
            return _fd_;
        }

        uint32_t crtc() const
        {
            return _crtc_;
        }

        uint32_t encoder() const
        {
            return _encoder_;
        }

        uint32_t connector() const
        {
            return _connector_;
        }

        uint32_t fb(uint32_t id = 0, bool update = false)// const
        {
            if(false != update)
            {
                _fb_ = id;
            }

            return _fb_;
        }

        uint32_t mode(uint32_t id = 0, bool update = false)// const
        {
            if(false != update)
            {
                _mode_ = id;
            }

            return _mode_;
        }


// TODO: maybe leave out the 'scan out' part
        uint32_t ScanOutBufferWidth() //const
        {
            uint32_t width = 0;

            if((nullptr != _buffer_) && (false != valid()))
            {
                width = gbm_bo_get_width(_buffer_);
            }

            return width;
        }

        uint32_t ScanOutBufferHeight() //const
        {
            uint32_t height = 0;

            if((nullptr != _buffer_) && (false != valid()))
            {
                height = gbm_bo_get_height(_buffer_);
            }

            return height;
        }

// TODO: should it be const?
        bool ScanOut(const struct gbm_surface*);

        // These created resources are automatically destroyed if gbm_device is destroyed
        static const struct gbm_surface* CreateRenderTargetFromUnderlyingHandle(const struct gbm_device* device, uint32_t width, uint32_t height) //const
        {
            if(nullptr != device)
            {
                return gbm_surface_create(const_cast<struct gbm_device*>(device), width, height, DRM_FORMAT_XRGB8888, GBM_BO_USE_SCANOUT /* presented on a screen */ | GBM_BO_USE_RENDERING /* used for rendering */);
            }

            return nullptr;
        }

        // The user can still decide to destroy a resource explicitly
// TODO: leaks in double buffer mode
        static bool DestroyRenderTargetFromUnderlyingHandle(const struct gbm_device*, struct gbm_surface* surface)
        {
            // The gbm_device* is currently unused

            if(nullptr != surface)
            {
                if(nullptr != FB.bo)
                {
//TODO: avoid tearing; shouldn't the FB be completely removed and replaced?
                    /* void */ gbm_surface_release_buffer(surface, FB.bo);

                    FB.bo = nullptr;
                }

                /* void */ gbm_surface_destroy(surface);

                surface = nullptr;

                return true;
            }

            return false;
        }

        const gbm_device* const UnderlyingHandle() const
        {
            return _device_;
        }

    private :

        ModeSet::TYPE _type_;

        int32_t _fd_; // This one is shared. What happends at destruction?
        uint32_t _crtc_;
        uint32_t _encoder_;
        uint32_t _connector_;

        uint32_t _fb_;
        uint32_t _mode_;

        gbm_device* _device_;
        // Initial scanout buffer
        gbm_bo* _buffer_;

        // Here: double buffering by default
        static struct FB // Previous (front) buffer
        {
            gbm_bo* bo;// = nullptr;
            uint32_t fb;// = 0;
        } FB;

//TODO:  use enum instead of uint32_t

        std::vector<std::string> getnodes(uint32_t type) const;
        // bitmask
        uint32_t getconnectors(uint32_t type) const;
        // bitmask
        uint32_t getencoders(uint32_t type) const;
        // bitmask
        uint32_t getcrtcs(bool valid) const;

        // Initial buffers
        bool InitializeScanOutBuffer();
        bool DeinitializeScanOutBuffer();
};

// TODO: singleton? (underlying) resources should'n be duplicated
class EnumeratedModeSets
{
    public :

        EnumeratedModeSets() : _sets_({ModeSet(ModeSet::TYPE::INITIAL), ModeSet(ModeSet::TYPE::CURRENT)})
        {}
        ~EnumeratedModeSets() = default;

        ModeSet& operator[](const ModeSet::TYPE index)
        {
//TODO: what todo if out of range
            return _sets_[static_cast<std::underlying_type<ModeSet::TYPE>::type>(index)];
        }

        bool Initialize();
        bool Deinitialize();

    private :

        // Effective after initialize and before deintialize
        bool EnableModeSet(const ModeSet& set);

        std::array<ModeSet, 2> _sets_;
};
