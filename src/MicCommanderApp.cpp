#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"

#include "AssetManager.h"
#include "MiniConfig.h"

#include "msp_cmn.h"
#include "msp_errors.h"
#include "speech_recognizer.h"

#define FRAME_LEN	640 
#define	BUFFER_SIZE	4096

const char* login_params = "appid = 57bb9136, work_dir = ."; // 登录参数，appid与msc库绑定,请勿随意改动
/*
* sub:				请求业务类型
* domain:			领域
* language:			语言
* accent:			方言
* sample_rate:		音频采样率
* result_type:		识别结果格式
* result_encoding:	结果编码格式
*/
const char* session_begin_params = "sub = iat, domain = iat, language = zh_cn, accent = mandarin, sample_rate = 16000, result_type = plain, result_encoding = gb2312";

using namespace ci;
using namespace ci::app;
using namespace std;

static char *g_result = NULL;
static unsigned int g_buffersize = BUFFER_SIZE;

void on_result(const char *result, char is_last)
{
    if (result) {
        size_t left = g_buffersize - 1 - strlen(g_result);
        size_t size = strlen(result);
        if (left < size) {
            g_result = (char*)realloc(g_result, g_buffersize + BUFFER_SIZE);
            if (g_result)
                g_buffersize += BUFFER_SIZE;
            else {
                CI_LOG_I("mem alloc failed");
                return;
            }
        }
        strncat(g_result, result, size);
        _WORD = g_result;
    }
}
void on_speech_begin()
{
    if (g_result)
    {
        free(g_result);
    }
    g_result = (char*)malloc(BUFFER_SIZE);
    g_buffersize = BUFFER_SIZE;
    memset(g_result, 0, g_buffersize);

    _STATUS = "Start Listening...";
}

void on_speech_end(int reason)
{
    if (reason == END_REASON_VAD_DETECT)
        _STATUS = "Speaking done";
    else
        _STATUS = "Recognizer error";
}

class MicCommanderApp : public App
{
    speech_rec iat;

public:
    void setup() override
    {
        log::makeLogger<log::LoggerFile>();

        int ret = MSPLogin(NULL, NULL, login_params); //第一个参数是用户名，第二个参数是密码，均传NULL即可，第三个参数是登录参数	
        if (MSP_SUCCESS != ret) {
            CI_LOG_E("MSPLogin failed , Error code " << ret);
            quit();
            return;
        }

        speech_rec_notifier recnotifier =
        {
            on_result,
            on_speech_begin,
            on_speech_end
        };

        int errcode = sr_init(&iat, session_begin_params, SR_MIC, DEFAULT_INPUT_DEVID, &recnotifier);
        if (errcode) {
            CI_LOG_E("speech recognizer init failed\n");
            quit();
            return;
        }

        auto params = createConfigUI({ 400, 200 });
        params->addButton("Start Record", [&] {
            if (int errcode = sr_start_listening(&iat)) {
                _STATUS = "Start listen failed";
                //quit();
            }
            else
            {
                _STATUS = "Started";
            }
        });

        params->addButton("Stop Record", [&] {
            if (int errcode = sr_stop_listening(&iat)) {
                _STATUS = "Stop listen failed";
                //quit();
            }
            else
            {
                _STATUS = "Stopped";
            }
        });

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE) quit();
        });

        getWindow()->getSignalDraw().connect([&] {
            gl::clear();
            gl::ScopedGlslProg glsl(am::glslProg("color"));
        });

        getSignalCleanup().connect([&] {
            sr_stop_listening(&iat);
            sr_uninit(&iat);
            MSPLogout();
        });
    }
};

CINDER_APP(MicCommanderApp, RendererGl, [](App::Settings* settings) {
    readConfig();
    _STATUS = "Waiting for your input.";
    settings->setWindowSize(_APP_WIDTH, _APP_HEIGHT);
    settings->setMultiTouchEnabled(false);
})
