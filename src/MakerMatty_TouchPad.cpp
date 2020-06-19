#ifdef ESP32

#include "MakerMatty_TouchPad.h"

TouchPad::TouchPad(uint8_t max_count)
    : touchPins(new TouchPin*[max_count])
    , taskData { .self = this, .dataSemaphore = nullptr, .pHardwareSemaphore = nullptr }
    , taskHandle(nullptr)
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

void TouchPad::attach(touch_pad_t pin, uint16_t tap_ms = 50, uint16_t press_ms = 750)
{
    if (!taskData.dataSemaphore || xSemaphoreTake(taskData.dataSemaphore, portMAX_DELAY)) {

        for (uint8_t i = 0; i < touchPinsAttached; i++) {
            if (touchPins[i]->getPin() == pin) {
                detach(pin);
                break;
            }
        }
        touchPins[touchPinsAttached++] = new TouchPin(pin, tap_ms, press_ms);

        if (taskData.dataSemaphore) {
            xSemaphoreGive(taskData.dataSemaphore);
        }
    }
}

void TouchPad::detach(touch_pad_t pin)
{
    if (!taskData.dataSemaphore || xSemaphoreTake(taskData.dataSemaphore, portMAX_DELAY)) {

        for (uint8_t i = 0; i < touchPinsAttached; i++) {
            if (touchPins[i]->getPin() == pin) {
                delete touchPins[i];
                touchPins[i] = touchPins[touchPinsAttached - 1];
                touchPinsAttached--;
                break;
            }
        }

        if (taskData.dataSemaphore) {
            xSemaphoreGive(taskData.dataSemaphore);
        }
    }
}

bool TouchPad::begin(BaseType_t xCoreID, SemaphoreHandle_t* pHwSemaphore)
{
    if (taskHandle == nullptr) {

        if (taskData.dataSemaphore) {
            vSemaphoreDelete(taskData.dataSemaphore);
            taskData.dataSemaphore = nullptr;
        }

        if (taskData.dataSemaphore == nullptr) {
            taskData.dataSemaphore = xSemaphoreCreateMutex();
            assert(taskData.dataSemaphore);
        }

        if (pHwSemaphore) {
            if (*pHwSemaphore == nullptr) {
                *pHwSemaphore = xSemaphoreCreateMutex();
                if (!*pHwSemaphore) {
                    assert(*pHwSemaphore);
                    return false;
                }
            }
            taskData.pHardwareSemaphore = pHwSemaphore;
        } else {
            taskData.pHardwareSemaphore = nullptr;
        }

        BaseType_t result = xTaskCreateUniversal(touchpadTask, "touchpad", 4096, &taskData, 1, &taskHandle, xCoreID);
        assert(result == pdPASS);
    }
}

void TouchPad::touchpadTask_update(void* p)
{
    TaskData* data = (TaskData*)p;

    for (uint8_t i = 0; i < data->self->touchPinsAttached; i++) {

        if (!data->pHardwareSemaphore || xSemaphoreTake(*(data->pHardwareSemaphore), 1)) {
#if MM_TOUCHPAD_DEBUG
            uint8_t reading = data->self->touchPins[i]->update();
            //uint8_t pin = data->self->touchPins[i]->getPin();
            Serial.printf("%u, ", reading);
#else
            data->self->touchPins[i]->update();
#endif
            if (data->pHardwareSemaphore) {
                xSemaphoreGive(*(data->pHardwareSemaphore));
            }
        }

        if (data->self->touchPins[i]->contacted()) {
            if (data->self->m_contactCb != nullptr) {
                (*data->self->m_contactCb)(data->self->touchPins[i]->getPin(), data->self->m_contactArg);
            }
        }
        if (data->self->touchPins[i]->released()) {
            if (data->self->m_releaseCb != nullptr) {
                (*data->self->m_releaseCb)(data->self->touchPins[i]->getPin(), data->self->m_releaseArg);
            }
        }
        if (data->self->touchPins[i]->tapped()) {
            if (data->self->m_tapCb != nullptr) {
                (*data->self->m_tapCb)(data->self->touchPins[i]->getPin(), data->self->m_tapArg);
            }
        }
        if (data->self->touchPins[i]->pressed()) {
            if (data->self->m_pressCb != nullptr) {
                (*data->self->m_pressCb)(data->self->touchPins[i]->getPin(), data->self->m_pressArg);
            }
        }
    }
#if MM_TOUCHPAD_DEBUG
    Serial.printf("\n");
#endif
}

void TouchPad::touchpadTask(void* p)
{
    TaskData* data = (TaskData*)p;

    while (1) {

        if (xSemaphoreTake(data->dataSemaphore, 1) == pdTRUE) {
            touchpadTask_update(p);
            xSemaphoreGive(data->dataSemaphore);
        }

        delay(10);
    }
}

void TouchPad::end()
{
    if (xSemaphoreTake(taskData.dataSemaphore, portMAX_DELAY) == pdTRUE) {
        if (taskData.dataSemaphore != nullptr) {
            vSemaphoreDelete(taskData.dataSemaphore);
            taskData.dataSemaphore = nullptr;
        }
        if (taskHandle != nullptr) {
            vTaskDelete(taskHandle);
            taskHandle = nullptr;
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
