#ifdef ESP32

#include "MakerMatty_TouchPad.h"

TouchPad::TouchPad(uint8_t max_count)
    : touchPins(new TouchPin*[max_count])
    , taskData { .taskHandle = nullptr, .semaphore = nullptr, .touchpad = this }
    , touchPinsAttached(0)
    , touchPinsMax(max_count)
    , m_contactCb(nullptr)
    , m_contactArg(nullptr)
    , m_releaseCb(nullptr)
    , m_releaseArg(nullptr)
    , m_tapCb(nullptr)
    , m_tapArg(nullptr)
    , m_pressCb(nullptr)
    , m_pressArg(nullptr)
{
}

void TouchPad::attach(touch_pad_t pin,  uint16_t tap_ms = 50, uint16_t press_ms = 750)
{
    bool give = false;
    if (taskData.semaphore != nullptr) {
        if (xSemaphoreTake(taskData.semaphore, portMAX_DELAY) == pdTRUE) {
            give = true;
        } else {
            abort();
        }
    }

    for (uint8_t i = 0; i < touchPinsAttached; i++) {
        if (touchPins[i]->getPin() == pin) {
            detach(pin);
            break;
        }
    }
    touchPins[touchPinsAttached++] = new TouchPin(pin, tap_ms, press_ms);

    if (give) {
        xSemaphoreGive(taskData.semaphore);
    }
}

void TouchPad::detach(touch_pad_t pin)
{
    bool give = false;
    if (taskData.semaphore != nullptr) {
        if (xSemaphoreTake(taskData.semaphore, portMAX_DELAY) == pdTRUE) {
            give = true;
        } else {
            abort();
        }
    }

    for (uint8_t i = 0; i < touchPinsAttached; i++) {
        if (touchPins[i]->getPin() == pin) {
            delete touchPins[i];
            touchPins[i] = touchPins[touchPinsAttached - 1];
            touchPinsAttached--;
            break;
        }
    }

    if (give) {
        xSemaphoreGive(taskData.semaphore);
    }
}

void TouchPad::begin(BaseType_t xCoreID)
{
    if (taskData.taskHandle == nullptr) {
        if (taskData.semaphore == nullptr) {
            taskData.semaphore = xSemaphoreCreateMutex();
            assert(taskData.semaphore);
        }

        BaseType_t result = xTaskCreateUniversal(touchpadTask, "touchpad", 4096, &taskData, 1, &taskData.taskHandle, xCoreID);
        assert(result == pdPASS);
    }
}

void TouchPad::touchpadTask(void* p)
{
    TaskData* data = (TaskData*)p;

    while (1) {

        if (xSemaphoreTake(data->semaphore, 0) == pdTRUE) {

            for (uint8_t i = 0; i < data->touchpad->touchPinsAttached; i++) {

#if MM_TOUCHPAD_DEBUG
                uint8_t reading = data->touchpad->touchPins[i]->update();
                //uint8_t pin = data->touchpad->touchPins[i]->getPin();
                Serial.printf("%u, ", reading);
#else
                data->touchpad->touchPins[i]->update();
#endif

                if (data->touchpad->touchPins[i]->contacted()) {
                    if (data->touchpad->m_contactCb != nullptr) {
                        (*data->touchpad->m_contactCb)(data->touchpad->touchPins[i]->getPin(), data->touchpad->m_contactArg);
                    }
                }
                if (data->touchpad->touchPins[i]->released()) {
                    if (data->touchpad->m_releaseCb != nullptr) {
                        (*data->touchpad->m_releaseCb)(data->touchpad->touchPins[i]->getPin(), data->touchpad->m_releaseArg);
                    }
                }
                if (data->touchpad->touchPins[i]->tapped()) {
                    if (data->touchpad->m_tapCb != nullptr) {
                        (*data->touchpad->m_tapCb)(data->touchpad->touchPins[i]->getPin(), data->touchpad->m_tapArg);
                    }
                }
                if (data->touchpad->touchPins[i]->pressed()) {
                    if (data->touchpad->m_pressCb != nullptr) {
                        (*data->touchpad->m_pressCb)(data->touchpad->touchPins[i]->getPin(), data->touchpad->m_pressArg);
                    }
                }
            }
#if MM_TOUCHPAD_DEBUG
            Serial.printf("\n");
#endif

            xSemaphoreGive(data->semaphore);
        }

        delay(10);
    }
}

void TouchPad::end()
{
    if (xSemaphoreTake(taskData.semaphore, portMAX_DELAY) == pdTRUE) {
        if (taskData.semaphore != nullptr) {
            vSemaphoreDelete(taskData.semaphore);
            taskData.semaphore = nullptr;
        }
        if (taskData.taskHandle != nullptr) {
            vTaskDelete(taskData.taskHandle);
            taskData.taskHandle = nullptr;
        }
    } else {
        abort();
    }
}

void TouchPad::onContact(TouchPadCallback cb, void* cbarg)
{
    m_contactCb = cb;
    m_contactArg = cbarg;
}
void TouchPad::onRelease(TouchPadCallback cb, void* cbarg)
{
    m_releaseCb = cb;
    m_releaseArg = cbarg;
}
void TouchPad::onTap(TouchPadCallback cb, void* cbarg)
{
    m_tapCb = cb;
    m_tapArg = cbarg;
}
void TouchPad::onPress(TouchPadCallback cb, void* cbarg)
{
    m_pressCb = cb;
    m_pressArg = cbarg;
}

TouchPad::~TouchPad()
{
    for (uint8_t i = 0; i < touchPinsAttached; i++) {
        delete touchPins[i];
    }
    delete[] touchPins;
}

#endif
