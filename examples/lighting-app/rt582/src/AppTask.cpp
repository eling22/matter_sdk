/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "AppTask.h"
#include "AppConfig.h"
#include "AppEvent.h"

#include <OTAConfig.h>

#include <app-common/zap-generated/attribute-id.h>
#include <app-common/zap-generated/attribute-type.h>
#include <app-common/zap-generated/cluster-id.h>
#include <app/clusters/identify-server/identify-server.h>
#include <app/clusters/on-off-server/on-off-server.h>
#include <app/server/OnboardingCodesUtil.h>
#include <app/server/Server.h>
#include <app/server/Dnssd.h>
#include <app/util/attribute-storage.h>

#include <assert.h>
#include <DeviceInfoProviderImpl.h>
#include <setup_payload/QRCodeSetupPayloadGenerator.h>
#include <setup_payload/SetupPayload.h>

#include "queue.h"

#include <lib/support/CodeUtils.h>
#include <platform/CHIPDeviceLayer.h>

#include <platform/CommissionableDataProvider.h>

#include <credentials/DeviceAttestationCredsProvider.h>
#include <credentials/examples/DeviceAttestationCredsExample.h>

#include <lib/core/CHIPError.h>

#include "uart.h"
#include "util_log.h"
#include "cm3_mcu.h"
#include "init_rt582Platform.h"
#include "init_lighting_rt582Platform.h"
#include "bsp.h"
#include "bsp_button.h"


using namespace chip;
using namespace chip::TLV;
using namespace ::chip::Credentials;
using namespace ::chip::DeviceLayer;


#define FACTORY_RESET_TRIGGER_TIMEOUT 6000
#define APP_TASK_STACK_SIZE (2 * 1024)
#define APP_TASK_PRIORITY 2
#define APP_EVENT_QUEUE_SIZE 10
#define LIGHT_ENDPOINT_ID (1)

namespace {
bool sIsThreadBLEAdvertising = false;
bool sIsThreadProvisioned = false;
bool sIsThreadEnabled     = false;
bool sHaveBLEConnections  = false;
bool sCommissioned        = false;
static TaskHandle_t sAppTaskHandle;
static QueueHandle_t sAppEventQueue;

static uint8_t sAppEventQueueBuffer[APP_EVENT_QUEUE_SIZE * sizeof(AppEvent)];

static StaticQueue_t sAppEventQueueStruct;

static StackType_t appStack[APP_TASK_STACK_SIZE / sizeof(StackType_t)];
static StaticTask_t appTaskStruct;

static DeviceInfoProviderImpl gExampleDeviceInfoProvider;

// Identify gIdentify = {
//     LIGHT_ENDPOINT_ID,
//     AppTask::IdentifyStartHandler,
//     AppTask::IdentifyStopHandler,
//     EMBER_ZCL_IDENTIFY_IDENTIFY_TYPE_VISIBLE_LIGHT,
// };


EmberAfIdentifyEffectIdentifier sIdentifyEffect = EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_STOP_EFFECT;

/**********************************************************
 * Identify Callbacks
 *********************************************************/

void OnTriggerIdentifyEffectCompleted(chip::System::Layer * systemLayer, void * appState)
{
    ChipLogProgress(Zcl, "Trigger Identify Complete");
    sIdentifyEffect = EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_STOP_EFFECT;

// #if CHIP_DEVICE_CONFIG_ENABLE_SED == 1
//     AppTask::GetAppTask().StopStatusLEDTimer();
// #endif

    rt582_led_level_ctl(2, 0);
    rt582_led_level_ctl(3, 0);
    rt582_led_level_ctl(4, 0);
}

void OnTriggerIdentifyEffectBlink(chip::System::Layer * systemLayer, void * appState)
{
    uint32_t i = 0;

    for (uint32_t cnt = 0; cnt < 1; cnt++) {

        rt582_led_level_ctl(2, 254);
        rt582_led_level_ctl(3, 254);
        rt582_led_level_ctl(4, 254);
        vTaskDelay(500);
        rt582_led_level_ctl(2, 0);
        rt582_led_level_ctl(3, 0);
        rt582_led_level_ctl(4, 0);
        vTaskDelay(500);
    }
}

void OnTriggerIdentifyEffectBreathe(chip::System::Layer * systemLayer, void * appState)
{
    uint32_t i = 0;

    for (uint32_t cnt = 0; cnt < 15; cnt++) {

        rt582_led_level_ctl(2, 254);
        rt582_led_level_ctl(3, 254);
        rt582_led_level_ctl(4, 254);
        vTaskDelay(50);
        rt582_led_level_ctl(2, 0);
        rt582_led_level_ctl(3, 0);
        rt582_led_level_ctl(4, 0);
        vTaskDelay(50);
    }
}

void OnTriggerIdentifyEffectOk(chip::System::Layer * systemLayer, void * appState)
{
    uint32_t i = 0;

    for (uint32_t cnt = 0; cnt < 2; cnt++) {

        rt582_led_level_ctl(2, 254);
        rt582_led_level_ctl(3, 254);
        rt582_led_level_ctl(4, 254);
        vTaskDelay(250);
        rt582_led_level_ctl(2, 0);
        rt582_led_level_ctl(3, 0);
        rt582_led_level_ctl(4, 0);
        vTaskDelay(250);
    }
}

void OnTriggerIdentifyEffectChannel(chip::System::Layer * systemLayer, void * appState)
{
    uint32_t i = 0;

    for (uint32_t cnt = 0; cnt < 1; cnt++) {

        rt582_led_level_ctl(2, 254);
        rt582_led_level_ctl(3, 254);
        rt582_led_level_ctl(4, 254);
        vTaskDelay(500);
        rt582_led_level_ctl(2, 20);
        rt582_led_level_ctl(3, 20);
        rt582_led_level_ctl(4, 20);
        vTaskDelay(1500);
    }
}

void OnTriggerIdentifyEffect(Identify * identify)
{
    sIdentifyEffect = identify->mCurrentEffectIdentifier;

// #if CHIP_DEVICE_CONFIG_ENABLE_SED == 1
//     AppTask::GetAppTask().StartStatusLEDTimer();
// #endif

    switch (sIdentifyEffect)
    {
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_BLINK:
        ChipLogProgress(Zcl, "EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_BLINK");
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(1), OnTriggerIdentifyEffectBlink, identify);
        break;
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_BREATHE:
        ChipLogProgress(Zcl, "EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_BREATHE");
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(1), OnTriggerIdentifyEffectBreathe, identify);
        break;
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_OKAY:
        ChipLogProgress(Zcl, "EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_OKAY");
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(1), OnTriggerIdentifyEffectOk, identify);
        break;
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_FINISH_EFFECT:
        ChipLogProgress(Zcl, "EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_FINISH_EFFECT");
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(1), OnTriggerIdentifyEffectCompleted, identify);
        break;
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_CHANNEL_CHANGE:
        ChipLogProgress(Zcl, "EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_CHANNEL_CHANGE");
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(1), OnTriggerIdentifyEffectChannel, identify);
        (void) chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Seconds16(2), OnTriggerIdentifyEffectCompleted, identify);
        break;
    case EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_STOP_EFFECT:
        ChipLogProgress(Zcl, "EMBER_ZCL_IDENTIFY_EFFECT_IDENTIFIER_STOP_EFFECT");
        (void) chip::DeviceLayer::SystemLayer().CancelTimer(OnTriggerIdentifyEffectCompleted, identify);
        break;    
    default:
        ChipLogProgress(Zcl, "No identifier effect");
    }
}

Identify gIdentify = {
    LIGHT_ENDPOINT_ID,
    AppTask::IdentifyStartHandler,
    AppTask::IdentifyStopHandler,
    EMBER_ZCL_IDENTIFY_IDENTIFY_TYPE_VISIBLE_LIGHT,
    OnTriggerIdentifyEffect,
};

} //namespace

constexpr EndpointId kNetworkCommissioningEndpointSecondary = 0xFFFE;
using namespace chip::TLV;
using namespace ::chip::DeviceLayer;
AppTask AppTask::sAppTask;

void LockOpenThreadTask(void)
{
    chip::DeviceLayer::ThreadStackMgr().LockThreadStack();
}

void UnlockOpenThreadTask(void)
{
    chip::DeviceLayer::ThreadStackMgr().UnlockThreadStack();
}

void AppTask::IdentifyStartHandler(Identify *)
{
    AppEvent event;
    event.Type               = AppEvent::kEventType_Identify_Start;
    event.Handler            = IdentifyHandleOp;
    sAppTask.PostEvent(&event);
}

void AppTask::IdentifyStopHandler(Identify *)
{
    AppEvent event;
    event.Type               = AppEvent::kEventType_Identify_Stop;
    event.Handler            = IdentifyHandleOp;
    sAppTask.PostEvent(&event);
}

void AppTask::PostAppIdentify()
{
    AppEvent event;
    event.Type               = AppEvent::kEventType_Identify_Identify;
    event.Handler            = IdentifyHandleOp;
    sAppTask.PostEvent(&event);    
}

void AppTask::IdentifyHandleOp(AppEvent * aEvent)
{
    static uint32_t identifyState = 0;

    // ChipLogProgress(NotSpecified, "identify effect = %x", aEvent->Type);

    if (aEvent->Type == AppEvent::kEventType_Identify_Start)
    {
        identifyState = 1;
        ChipLogProgress(NotSpecified, "identify start");
    }

    else if (aEvent->Type == AppEvent::kEventType_Identify_Identify && identifyState)
    {
        rt582_led_level_ctl(2, 254);
        rt582_led_level_ctl(3, 254);
        rt582_led_level_ctl(4, 254);

        ChipLogProgress(NotSpecified, "identify");
    }

    else if (aEvent->Type == AppEvent::kEventType_Identify_Stop)
    {
        identifyState = 0;
        rt582_led_level_ctl(2, 0);
        rt582_led_level_ctl(3, 0);
        rt582_led_level_ctl(4, 0);
        ChipLogProgress(NotSpecified, "identify stop");
    }
}

/**
 * Update cluster status after application level changes
 */
void AppTask::UpdateClusterState(intptr_t arg)
{
    ChipLogProgress(NotSpecified, "UpdateClusterState");

    // write the new on/off value
    EmberAfStatus status = OnOffServer::Instance().setOnOffValue(1, LightMgr().IsTurnedOn(), false);

    if (status != EMBER_ZCL_STATUS_SUCCESS)
    {
        ChipLogProgress(NotSpecified, "ERR: updating on/off %x", status);
    }
}

void AppTask::ActionInitiated(LightingManager::Action_t aAction)
{
    // Placeholder for light action
    if (aAction == LightingManager::ON_ACTION)
    {
        ChipLogProgress(NotSpecified, "Light goes on");
    }
    else if (aAction == LightingManager::OFF_ACTION)
    {
        ChipLogProgress(NotSpecified, "Light goes off ");
    }

    if (sAppTask.mSyncClusterToButtonAction)
    {
        PlatformMgr().ScheduleWork(UpdateClusterState, 0);
        sAppTask.mSyncClusterToButtonAction = false;
    }
}

void AppTask::ActionCompleted(LightingManager::Action_t aAction)
{
    // Placeholder for light action completed
    uint8_t current_level = 0;
    RgbColor_t RGB;
    if (aAction == LightingManager::ON_ACTION)
    {
        ChipLogProgress(NotSpecified, "Light On Action has been completed");

        current_level = LightMgr().GetLevel();
        RGB = LightMgr().GetRgb();
        rt582_led_level_ctl(2, RGB.b);
        rt582_led_level_ctl(3, RGB.r);
        rt582_led_level_ctl(4, RGB.g);
    }
    else if (aAction == LightingManager::OFF_ACTION)
    {
        ChipLogProgress(NotSpecified, "Light Off Action has been completed");
        rt582_led_level_ctl(2, 0);
        rt582_led_level_ctl(3, 0);
        rt582_led_level_ctl(4, 0); 
    }

    // if (sAppTask.mSyncClusterToButtonAction)
    // {
    //     chip::DeviceLayer::PlatformMgr().ScheduleWork(UpdateClusterState(), reinterpret_cast<intptr_t>(nullptr));
    //     sAppTask.mSyncClusterToButtonAction = false;
    // }
}
void AppTask::LightActionEventHandler(AppEvent * aEvent)
{
    bool initiated = false;
    LightingManager::Action_t action;
    int32_t actor;
    CHIP_ERROR err = CHIP_NO_ERROR;

    if (aEvent->Type == AppEvent::kEventType_Light)
    {
        action = static_cast<LightingManager::Action_t>(aEvent->LightEvent.Action);
        actor  = aEvent->LightEvent.Actor;
        // Toggle Dimming of light between 2 fixed levels
        uint8_t val = 0x0;
        val         = LightMgr().GetLevel() == 0x7f ? 0x1 : 0x7f;
        action      = LightingManager::LEVEL_ACTION;
        initiated = LightMgr().InitiateAction(action, 0, 1, &val);

        if (!initiated)
        {
            ChipLogProgress(NotSpecified, "Action is already in progress or active.");
        }
    }
    else if (aEvent->Type == AppEvent::kEventType_Button)
    {
        action = (LightMgr().IsTurnedOn()) ? LightingManager::OFF_ACTION : LightingManager::ON_ACTION;
        actor  = AppEvent::kEventType_Button;
        // Toggle Dimming of light between 2 fixed levels
        uint8_t val = (action == LightingManager::ON_ACTION) ? 0x7f : 0x1;
        initiated = LightMgr().InitiateAction(action, 0, 1, &val);

        if (!initiated)
        {
            ChipLogProgress(NotSpecified, "Action is already in progress or active.");
        }
    }
    else
    {
        err = APP_ERROR_UNHANDLED_EVENT;
    }
}

void AppTask::InitServer(intptr_t arg)
{
    
#if 1
    CHIP_ERROR err;
    static chip::CommonCaseDeviceServerInitParams initParams;
    (void) initParams.InitializeStaticResourcesBeforeServerInit();

    gExampleDeviceInfoProvider.SetStorageDelegate(initParams.persistentStorageDelegate);
    SetDeviceInfoProvider(&gExampleDeviceInfoProvider);

    chip::Inet::EndPointStateOpenThread::OpenThreadEndpointInitParam nativeParams;
    nativeParams.lockCb                = LockOpenThreadTask;
    nativeParams.unlockCb              = UnlockOpenThreadTask;
    nativeParams.openThreadInstancePtr = chip::DeviceLayer::ThreadStackMgrImpl().OTInstance();
    initParams.endpointNativeParams    = static_cast<void *>(&nativeParams);

    err = chip::Server::GetInstance().Init(initParams);
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "chip::Server::init faild %s", ErrorStr(err));
    }
    if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
    {
        vTaskSuspendAll();
        PrintOnboardingCodes(chip::RendezvousInformationFlags(chip::RendezvousInformationFlag::kBLE));
        xTaskResumeAll();
    }
    else
    {
        chip::app::DnssdServer::Instance().StartServer();
        sCommissioned = true;
        UpdateStatusLED();        
    }
#endif
    // Setup light
    err = LightMgr().Init();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "LightingMgr().Init() failed");
        //return err;
    }
    LightMgr().SetCallbacks(ActionInitiated, ActionCompleted);

    // uint8_t current_level = 0;
    // RgbColor_t RGB;

    // if (LightMgr().IsTurnedOn())
    // {
    //     current_level = LightMgr().GetLevel();
    //     RGB = LightMgr().GetRgb();
    //     rt582_led_level_ctl(2, RGB.b);
    //     rt582_led_level_ctl(3, RGB.r);
    //     rt582_led_level_ctl(4, RGB.g);
    // }
    // else
    // {
    //     rt582_led_level_ctl(2, 0); 
    //     rt582_led_level_ctl(3, 0); 
    //     rt582_led_level_ctl(4, 0);      
    // }

#if RT582_OTA_ENABLED
    OTAConfig::Init();
#endif
}

void AppTask::UpdateStatusLED()
{
#if(CHIP_DEVICE_CONFIG_ENABLE_SED == 0)    
    if (sIsThreadBLEAdvertising && !sHaveBLEConnections)
    {
        init_rt582_led_flash(20, 250, 150);
    }
    else if (sIsThreadProvisioned && sIsThreadEnabled)
    {
        init_rt582_led_flash(20, 850, 150);
    }
    else if (sHaveBLEConnections)
    {
        init_rt582_led_flash(20, 50, 50);
    }
#endif    
}

void AppTask::ChipEventHandler(const ChipDeviceEvent * aEvent, intptr_t /* arg */)
{
    ChipLogProgress(NotSpecified, "ChipEventHandler: %x", aEvent->Type);
    switch (aEvent->Type)
    {
    case DeviceEventType::kCHIPoBLEAdvertisingChange:
        sIsThreadBLEAdvertising = true;
        UpdateStatusLED();   
        break;
    case DeviceEventType::kCHIPoBLEConnectionClosed:
    case DeviceEventType::kFailSafeTimerExpired:
        sHaveBLEConnections = false;
        UpdateStatusLED();
        break;
    case DeviceEventType::kThreadStateChange:
        sIsThreadProvisioned = ConnectivityMgr().IsThreadProvisioned();
        sIsThreadEnabled     = ConnectivityMgr().IsThreadEnabled();
        UpdateStatusLED();
        break;
    case DeviceEventType::kThreadConnectivityChange:
        break;
    case DeviceEventType::kCHIPoBLEConnectionEstablished:
        sHaveBLEConnections = true;
        UpdateStatusLED(); 
        break;
    case DeviceEventType::kCommissioningComplete:
        sCommissioned = true;
        UpdateStatusLED();
        break;
    case DeviceEventType::kRemoveFabricEvent:
        if (chip::Server::GetInstance().GetFabricTable().FabricCount() == 0)
        {
            chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Milliseconds32(1000), FactoryResetEventHandler, nullptr);
        }
        break;
    case DeviceEventType::kOnOffAttributeChanged:
        LightMgr().InitiateAction(aEvent->OnOffChanged.value ? LightingManager::ON_ACTION : LightingManager::OFF_ACTION, 
                                  0, 
                                  sizeof(aEvent->OnOffChanged.value), 
                                  (uint8_t *)&aEvent->OnOffChanged.value);
        break;
    case DeviceEventType::kLevelControlAttributeChanged:
        LightMgr().InitiateAction(LightingManager::LEVEL_ACTION, 
                                  0, 
                                  sizeof(aEvent->LevelControlChanged.level), 
                                  (uint8_t *)&aEvent->LevelControlChanged.level);
        break;
    case DeviceEventType::kColorControlAttributeHSVChanged:
        LightMgr().InitiateAction(LightingManager::COLOR_ACTION_HSV, 
                                  0, 
                                  sizeof(aEvent->ColorControlHSVChanged), 
                                  (uint8_t *)&aEvent->ColorControlHSVChanged);
        break;
    case DeviceEventType::kColorControlAttributeCTChanged:
        LightMgr().InitiateAction(LightingManager::COLOR_ACTION_CT, 
                                  0, 
                                  sizeof(aEvent->ColorControlCTChanged), 
                                  (uint8_t *)&aEvent->ColorControlCTChanged);
        break;
    default:
        break;
    }
}

CHIP_ERROR AppTask::Init()
{
    CHIP_ERROR err;
    // ChipLogProgress(NotSpecified, "Current Software Version: %s", CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION_STRING);
    
    // chip::DeviceLayer::ConfigurationMgr().StoreSoftwareVersion(CHIP_DEVICE_CONFIG_DEVICE_SOFTWARE_VERSION);

    FactoryResetCheck();

    err = ThreadStackMgr().InitThreadStack();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "ThreadStackMgr().InitThreadStack() failed");
    }

    ChipLogProgress(NotSpecified, "Device Type : 0x%04X", CHIP_DEVICE_CONFIG_DEVICE_TYPE);

#if CHIP_DEVICE_CONFIG_ENABLE_SED
    err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_SleepyEndDevice);
#else
    err = ConnectivityMgr().SetThreadDeviceType(ConnectivityManager::kThreadDeviceType_Router);
#endif
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "ConnectivityMgr().SetThreadDeviceType() failed");
    }

    ChipLogProgress(NotSpecified, "Start Thread Task\n");
    err = ThreadStackMgr().StartThreadTask();
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "ThreadStackMgr().InitThreadStack() failed");
    }

    if (PlatformMgr().StartEventLoopTask() != CHIP_NO_ERROR)
    {
       ChipLogError(NotSpecified, "Error during PlatformMgr().StartEventLoopTask();");
    }
    PlatformMgr().ScheduleWork(InitServer, 0);
    PlatformMgr().AddEventHandler(ChipEventHandler, 0);

    return CHIP_NO_ERROR;
}

void AppTask::FactoryResetCheck()
{
    uint32_t i =0, reboot_cnt;

    ConfigurationMgr().GetRebootCount(reboot_cnt);
    ChipLogProgress(NotSpecified, "Cur reboot cnt : %d", reboot_cnt);

    if(reboot_cnt >= 6)
    {
        ChipLogProgress(NotSpecified, "[Reboot] - Factory Reset (>6)" );

        for(i=0;i<3;i++)
        {
            rt582_led_level_ctl(2, 120);
            rt582_led_level_ctl(3, 120);
            rt582_led_level_ctl(4, 120);
            Delay_ms(500);
            rt582_led_level_ctl(2, 250);
            rt582_led_level_ctl(3, 250);
            rt582_led_level_ctl(4, 250);
            Delay_ms(500);
        }
        chip::Server::GetInstance().ScheduleFactoryReset();
 
    }
    else
    {
        reboot_cnt++;
        ConfigurationMgr().StoreRebootCount(reboot_cnt);
    }


    sAppTask.StartTimer(FACTORY_RESET_TRIGGER_TIMEOUT);
    sAppTask.mFunction = kFunction_ClearRebootCnt;

}

CHIP_ERROR AppTask::StartAppTask()
{
    CHIP_ERROR err;

    bsp_init(BSP_INIT_LEDS | BSP_INIT_BUTTONS, ButtonEventHandler);

    sAppEventQueue = xQueueCreateStatic(APP_EVENT_QUEUE_SIZE, sizeof(AppEvent), 
                                    sAppEventQueueBuffer, &sAppEventQueueStruct);
    if (sAppEventQueue == nullptr)
    {
        ChipLogError(NotSpecified, "Failed to allocate app event queue");
        return CHIP_ERROR_NO_MEMORY;
    }

    // Start App task.
    sAppTaskHandle = xTaskCreateStatic(AppTaskMain, APP_TASK_NAME, 
                                    ArraySize(appStack), nullptr, 1, appStack, &appTaskStruct);
    if (sAppTaskHandle == nullptr)
    {
        return CHIP_ERROR_NO_MEMORY;
    }
    
#if RAFAEL_CERTS_ENABLED
    ReturnErrorOnFailure(mFactoryDataProvider.Init());
    // SetDeviceInstanceInfoProvider(&mFactoryDataProvider);
    SetCommissionableDataProvider(&mFactoryDataProvider);
    SetDeviceAttestationCredentialsProvider(&mFactoryDataProvider);    
#else
    SetDeviceAttestationCredentialsProvider(Examples::GetExampleDACProvider());
#endif

    return CHIP_NO_ERROR;
}

void AppTask::FactoryResetEventHandler(chip::System::Layer * aLayer, void * aAppState)
{
    chip::Server::GetInstance().ScheduleFactoryReset();
}

void AppTask::TimerEventHandler(chip::System::Layer * aLayer, void * aAppState)
{
    AppEvent event;
    event.Type               = AppEvent::kEventType_Timer;
    event.TimerEvent.Context = aAppState;
    event.Handler            = FunctionTimerEventHandler;
    sAppTask.PostEvent(&event);
}


void AppTask::CancelTimer()
{
    chip::DeviceLayer::SystemLayer().CancelTimer(TimerEventHandler, this);
    mFunctionTimerActive = false;
}

void AppTask::StartTimer(uint32_t aTimeoutInMs)
{
    CHIP_ERROR err;

    chip::DeviceLayer::SystemLayer().CancelTimer(TimerEventHandler, this);
    err = chip::DeviceLayer::SystemLayer().StartTimer(chip::System::Clock::Milliseconds32(aTimeoutInMs), TimerEventHandler, this);
    SuccessOrExit(err);

    mFunctionTimerActive = true;
exit:
    if (err != CHIP_NO_ERROR)
    {
        ChipLogError(NotSpecified, "StartTimer failed %s: ", chip::ErrorStr(err));
    }
}

void AppTask::PostEvent(const AppEvent * aEvent)
{
    if (sAppEventQueue != NULL)
    {
        if (!xQueueSend(sAppEventQueue, aEvent, 1))
        {
        }
    }
}

void AppTask::DispatchEvent(AppEvent * aEvent)
{
    if (aEvent->Handler)
    {
        aEvent->Handler(aEvent);
    }
    else
    {
        ChipLogError(NotSpecified, "Event received with no handler. Dropping event.");
    }
}

void AppTask::FunctionTimerEventHandler(AppEvent * aEvent)
{
    uint32_t rebootCount = 0;
    if (aEvent->Type != AppEvent::kEventType_Timer)
    {
        return;
    }

    // If we reached here, the button was held past FACTORY_RESET_TRIGGER_TIMEOUT,
    // initiate factory reset
    else if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_FactoryReset)
    {
        // Actually trigger Factory Reset
        sAppTask.mFunction = kFunction_NoneSelected;
        chip::Server::GetInstance().ScheduleFactoryReset();
    }

    else if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_ClearRebootCnt)
    {
        sAppTask.mFunction = kFunction_NoneSelected;
        rebootCount = 0;
        ConfigurationMgr().StoreRebootCount(rebootCount);
        sAppTask.CancelTimer();
    }
}

void AppTask::FunctionHandler(AppEvent * aEvent)
{
    switch (aEvent->ButtonEvent.ButtonIdx)
    {
    case (AppEvent::AppActionTypes::kActionTypes_FactoryReset):
        if (aEvent->ButtonEvent.Action == true)
        {
            if (!sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_NoneSelected)
            {
                ChipLogProgress(NotSpecified, "[BTN] Hold to select function:");
                ChipLogProgress(NotSpecified, "[BTN] - Factory Reset (>6s)");

                sAppTask.StartTimer(FACTORY_RESET_TRIGGER_TIMEOUT);

                sAppTask.mFunction = kFunction_FactoryReset;
            }
        }
        else
        {
            if (sAppTask.mFunctionTimerActive && sAppTask.mFunction == kFunction_FactoryReset)
            {
                sAppTask.CancelTimer();

                // Change the function to none selected since factory reset has been
                // canceled.
                sAppTask.mFunction = kFunction_NoneSelected;

                ChipLogProgress(NotSpecified, "[BTN] Factory Reset has been Canceled");
            }
        }
    
    case (AppEvent::AppActionTypes::kActionTypes_Switch_1):
        if (aEvent->ButtonEvent.Action == true)
        {
            if (!sAppTask.mFunctionSwitchActive && sAppTask.mFunction == kFunction_NoneSelected)
            {
                sAppTask.mFunction = kFunction_Switch_1;
                sAppTask.mFunctionSwitchActive = true;
            }
        }
        else
        {
            if (sAppTask.mFunctionSwitchActive && sAppTask.mFunction == kFunction_Switch_1)
            {
                sAppTask.mSyncClusterToButtonAction = true;
                AppEvent event;
                event.Type               = AppEvent::kEventType_Button;
                event.Handler            = LightActionEventHandler;
                sAppTask.PostEvent(&event);

                sAppTask.mFunction = kFunction_NoneSelected;
                sAppTask.mFunctionSwitchActive = false;
            }
        }
        break;
    
    default:
        break;
    }
}

void AppTask::ButtonEventHandler(bsp_event_t event)
{
    // ChipLogProgress(NotSpecified, "ButtonEventHandler %d %d", (event), bsp_button_state_get(event-5));
    switch (event)
    {
    case (BSP_EVENT_BUTTONS_0):
        {
            AppEvent button_event              = {};
            button_event.Type                  = AppEvent::kEventType_Button;
            button_event.ButtonEvent.ButtonIdx = AppEvent::AppActionTypes::kActionTypes_FactoryReset;
            button_event.ButtonEvent.Action    = bsp_button_state_get(BSP_EVENT_BUTTONS_0-5)?0:1;
            // Hand off to Functionality handler - depends on duration of press
            button_event.Handler = FunctionHandler;
            xQueueSendFromISR(sAppEventQueue, &button_event, NULL);
        }
        break;
    
    case (BSP_EVENT_BUTTONS_1):
        {
            AppEvent button_event              = {};
            button_event.Type                  = AppEvent::kEventType_Button;
            button_event.ButtonEvent.ButtonIdx = AppEvent::AppActionTypes::kActionTypes_Switch_1;
            button_event.ButtonEvent.Action    = bsp_button_state_get(BSP_EVENT_BUTTONS_1-5)?0:1;
            button_event.Handler = FunctionHandler;
            xQueueSendFromISR(sAppEventQueue, &button_event, NULL);
        }
        break;
    default:
        break;
    }
}


void AppTask::AppTaskMain(void * pvParameter)
{
    AppEvent event;
    CHIP_ERROR err = sAppTask.Init();
    if (err != CHIP_NO_ERROR)
    {
        return ;
    }

    while (true)
    {
        BaseType_t eventReceived = xQueueReceive(sAppEventQueue, &event, portMAX_DELAY);
       
        while (eventReceived == pdTRUE)
        {
            sAppTask.DispatchEvent(&event);
            eventReceived = xQueueReceive(sAppEventQueue, &event, 0);
        }
    }
}
