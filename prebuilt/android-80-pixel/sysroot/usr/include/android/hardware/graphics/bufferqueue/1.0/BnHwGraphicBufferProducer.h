#ifndef HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_BUFFERQUEUE_V1_0_BNHWGRAPHICBUFFERPRODUCER_H
#define HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_BUFFERQUEUE_V1_0_BNHWGRAPHICBUFFERPRODUCER_H

#include <android/hardware/graphics/bufferqueue/1.0/IHwGraphicBufferProducer.h>

namespace android {
namespace hardware {
namespace graphics {
namespace bufferqueue {
namespace V1_0 {

struct BnHwGraphicBufferProducer : public ::android::hidl::base::V1_0::BnHwBase {
    explicit BnHwGraphicBufferProducer(const ::android::sp<IGraphicBufferProducer> &_hidl_impl);
    explicit BnHwGraphicBufferProducer(const ::android::sp<IGraphicBufferProducer> &_hidl_impl, const std::string& HidlInstrumentor_package, const std::string& HidlInstrumentor_interface);

    ::android::status_t onTransact(
            uint32_t _hidl_code,
            const ::android::hardware::Parcel &_hidl_data,
            ::android::hardware::Parcel *_hidl_reply,
            uint32_t _hidl_flags = 0,
            TransactCallback _hidl_cb = nullptr) override;

    ::android::sp<IGraphicBufferProducer> getImpl() { return _hidl_mImpl; };
private:
    // Methods from IGraphicBufferProducer follow.

    // Methods from ::android::hidl::base::V1_0::IBase follow.
    ::android::hardware::Return<void> ping();
    using getDebugInfo_cb = ::android::hidl::base::V1_0::IBase::getDebugInfo_cb;
    ::android::hardware::Return<void> getDebugInfo(getDebugInfo_cb _hidl_cb);

    ::android::sp<IGraphicBufferProducer> _hidl_mImpl;
};

}  // namespace V1_0
}  // namespace bufferqueue
}  // namespace graphics
}  // namespace hardware
}  // namespace android

#endif  // HIDL_GENERATED_ANDROID_HARDWARE_GRAPHICS_BUFFERQUEUE_V1_0_BNHWGRAPHICBUFFERPRODUCER_H
