/** 
 * Author	: @makermatty (maker.matejsuchanek.cz)
 * Date		: 15-6-2020
 */

#ifdef ESP32

#ifndef _MM_TOUCHPAD_h
#define _MM_TOUCHPAD_h

#include "MakerMatty_TouchPin.h"

#ifndef MM_TOUCHPAD_DEBUG
#define MM_TOUCHPAD_DEBUG 0
#endif

class TouchPad {
public:
    TouchPad(uint8_t max_count = TOUCH_PAD_MAX);
    ~TouchPad();

    void attach(touch_pad_t pin, uint16_t tap_ms = 50, uint16_t press_ms = 750, uint8_t knock_count = 3);
    void detach(touch_pad_t pin);

    bool begin(BaseType_t xCoreID = APP_CPU_NUM, SemaphoreHandle_t* pHwSemaphore = nullptr);
    void end();

    typedef void (*TouchPadCallback)(const touch_pad_t pin, void* cbarg);

    void onContact(TouchPadCallback cb, void* cbarg = nullptr);
    void onRelease(TouchPadCallback cb, void* cbarg = nullptr);
    void onTap(TouchPadCallback cb, void* cbarg = nullptr);
    void onPress(TouchPadCallback cb, void* cbarg = nullptr);

private:
    TouchPin** touchPins;

    struct TaskData {
        TouchPad* self;
        SemaphoreHandle_t dataSemaphore;
        SemaphoreHandle_t* pHardwareSemaphore;
    };

    TaskData taskData;
    TaskHandle_t taskHandle;
    static void touchpadTask_update(void* p);
    static void touchpadTask(void* p);

    uint8_t touchPinsAttached;
    uint8_t touchPinsMax;

    TouchPadCallback m_contactCb;
    void* m_contactArg;
    TouchPadCallback m_releaseCb;
    void* m_releaseArg;
    TouchPadCallback m_tapCb;
    void* m_tapArg;
    TouchPadCallback m_pressCb;
    void* m_pressArg;
};

typedef TouchPad MakerMatty_TouchPad;

#endif
#endif