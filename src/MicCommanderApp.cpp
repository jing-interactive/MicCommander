#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/CameraUi.h"
#include "cinder/Log.h"
#include "cinder/params/Params.h"
#include "cinder/Font.h"

#include "OscHelper.h"

#include "AssetManager.h"
#include "MiniConfig.h"
#include "AnsiToUtf.h"

#include "msp_cmn.h"
#include "msp_errors.h"
#include "speech_recognizer.h"

using namespace ci;
using namespace ci::app;
using namespace std;

unique_ptr<osc::SenderUdp> mOscSender;
unique_ptr<osc::ReceiverUdp> mOscReceiver;
speech_rec  iat;
string g_word;

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

void on_result(const char *result, char is_last)
{
    if (result)
    {
        if (is_last)
        {
            {
                osc::Message msg("/word/gb2312");
                msg.append(g_word);
                mOscSender->send(msg);
                CI_LOG_I(g_word);
            }

            {
                osc::Message msg("/word/utf8");
                _WORD = AnsiToUtf8(g_word);
                msg.append(_WORD);
                mOscSender->send(msg);
            }
        }
        else
        {
            g_word += result;
        }
    }
}

void on_speech_begin()
{
    _WORD = g_word = "";
    _STATUS = "Start Listening...";
}

void on_speech_end(int reason)
{
    if (reason == END_REASON_VAD_DETECT)
        _STATUS = "Speaking done";
    else
        _STATUS = "Recognizer error";
}

void startRecording()
{
    if (int errcode = sr_start_listening(&iat))
    {
        _STATUS = "Start listen failed";
    }
    else
    {
        _STATUS = "Started";
    }
}

void endRecording()
{
    if (int errcode = sr_stop_listening(&iat))
    {
        _STATUS = "Stop listen failed";
    }
    else
    {
        _STATUS = "Stopped";
    }
}

class MicCommanderApp : public App
{
    ci::Font mFontCN;

public:
    void setup() override
    {
        log::makeLogger<log::LoggerFile>();

        mOscSender = OscHelper::createSender(_REMOTE_IP, _REMOTE_PORT);
        mOscReceiver = OscHelper::createReceiver(_LOCAL_PORT);
        mOscReceiver->setListener("/start", [&](const osc::Message& message) {
            startRecording();
        });
        mOscReceiver->setListener("/end", [&](const osc::Message& message) {
            endRecording();
        });

        int ret = MSPLogin(NULL, NULL, login_params); //第一个参数是用户名，第二个参数是密码，均传NULL即可，第三个参数是登录参数	
        if (MSP_SUCCESS != ret)
        {
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
        if (errcode)
        {
            CI_LOG_E("speech recognizer init failed\n");
            quit();
            return;
        }

        auto params = createConfigUI({ 400, 250 });
        params->addButton("/start", startRecording);
        params->addButton("/end", endRecording);
        params->addText("Input: /start and /end");
        params->addText("Output: /word/gb2312 and /word/utf8");

        auto kMSYH = AnsiToUtf8("微软雅黑");
        mFontCN = Font(kMSYH, 24);

        getWindow()->getSignalKeyUp().connect([&](KeyEvent& event) {
            if (event.getCode() == KeyEvent::KEY_ESCAPE) quit();
        });

        getWindow()->getSignalDraw().connect([&] {
            gl::setMatricesWindow(getWindowSize());
            gl::clear();
            gl::ScopedGlslProg glsl(am::glslProg("color"));

            gl::drawString(_WORD, { 30,300 }, ColorA::white(), mFontCN);
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
