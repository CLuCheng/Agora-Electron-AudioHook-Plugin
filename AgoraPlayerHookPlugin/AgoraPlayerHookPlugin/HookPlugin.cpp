#include "HookPlugin.h"
#include "CicleBuffer.h"
#include <stdint.h>
#include "common/rapidjson/document.h"
#include "common/rapidjson/writer.h"
#include <tchar.h>
using namespace rapidjson;
FILE* outfile1 = NULL;
FILE* outfile = NULL;
CAudioCaptureCallback::CAudioCaptureCallback()
{
    m_lpHookAudioCicleBuffer = new CicleBuffer(44100 * 2 * 2, 0);
    m_lpHookAudioCicleBuffer->flushBuffer();
}

CAudioCaptureCallback::~CAudioCaptureCallback()
{
    if (m_lpHookAudioCicleBuffer) {
        delete[] m_lpHookAudioCicleBuffer;
        m_lpHookAudioCicleBuffer = nullptr;
    }
}

void CAudioCaptureCallback::onCaptureStart()
{

}

void CAudioCaptureCallback::onCaptureStop()
{
}

void CAudioCaptureCallback::onCapturedData(void* data, UINT dataLen, WAVEFORMATEX* format)
{
    m_lpHookAudioCicleBuffer->writeBuffer(data, dataLen);
}

//HookPlugin
CHookPlugin::CHookPlugin()
    : bForceRestartPlayer(false)
    , musicPlayerPath("")
{
    pPlayerData = new BYTE[0x800000];

    TCHAR szFilePath[MAX_PATH];
    ::GetModuleFileName(NULL, szFilePath, MAX_PATH);
    LPTSTR lpLastSlash = _tcsrchr(szFilePath, _T('\\'));

    if (lpLastSlash == NULL)
        return;

    SIZE_T nNameLen = MAX_PATH - (lpLastSlash - szFilePath + 1);
    _tcscpy_s(lpLastSlash + 1, nNameLen, _T("DebugMode.ini"));

    //Default 0
    TCHAR savePcm[MAX_PATH] = { 0 };
    ::GetPrivateProfileString(_T("DebugMode"), _T("SaveDumpPcm"), NULL, savePcm, MAX_PATH, "./DebugMode.ini");

    TCHAR DebugMode[MAX_PATH] = { 0 };
    ::GetPrivateProfileString(_T("DebugMode"), _T("DebugMode"), NULL, DebugMode, MAX_PATH, szFilePath);

    if (_tccmp(savePcm, _T("1")) == 0)
        isSaveDump = true;
    else
        isSaveDump = false;

    if (_tccmp(DebugMode, _T("1")) == 0)
        isDebugMode = true;
    else
        isDebugMode = false;

    if (isSaveDump) {
        fopen_s(&outfile1,"./AgoraHookLog/MusicDest.pcm", "ab+");
        fopen_s(&outfile,"./AgoraHookLog/FrameMix.pcm", "ab+");
    }
}

CHookPlugin::~CHookPlugin()
{
    if (isSaveDump) {
        if (outfile1) {
            fclose(outfile1);
            outfile1 = NULL;
        }

        if (outfile) {
            fclose(outfile);
            outfile = NULL;
        }
    }

    if (pPlayerData) {
        delete[] pPlayerData;
        pPlayerData = NULL;
    }
}

int16_t MixerAddS16(int16_t var1, int16_t var2)
{
    static const int32_t kMaxInt16 = 32767;
    static const int32_t kMinInt16 = -32768;
    int32_t tmp = (int32_t)var1 + (int32_t)var2;
    int16_t out16;

    if (tmp > kMaxInt16) {
        out16 = kMaxInt16;
    }
    else if (tmp < kMinInt16) {
        out16 = kMinInt16;
    }
    else {
        out16 = (int16_t)tmp;
    }

    return out16;
}

void MixerAddS16(int16_t* src1, const int16_t* src2, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        src1[i] = MixerAddS16(src1[i], src2[i]);
    }
}

bool CHookPlugin::onPluginRecordAudioFrame(AudioPluginFrame* audioFrame)
{
				if (!audioFrame) return true;

	SIZE_T nSize = audioFrame->channels*audioFrame->samples * 2;

    //for debug
    if(isDebugMode){

        static int nCountAudioCallBack = 0;
        nCountAudioCallBack++;
        static DWORD dwLastStamp = GetTickCount();
        DWORD dwCurrStamp = GetTickCount();
        if (5000 < dwCurrStamp - dwLastStamp) 
        {

            float fRecordAudioFrame = nCountAudioCallBack * 1000.0 / (dwCurrStamp - dwLastStamp);
            char logMsg[128] = { '\0' };
            sprintf_s(logMsg, "RecordAudioFrame :%d , %d,16,%d [ Rate : %.2f]\n", nSize, audioFrame->channels, audioFrame->samplesPerSec, fRecordAudioFrame);
            OutputDebugStringA(logMsg);

            FILE* log;
            fopen_s(&log,"./AgoraHookLog/PlayerHookerV6_1.log", ("a+"));
            if (log != NULL)
            {
                SYSTEMTIME st;
                GetLocalTime(&st);
                fprintf(log, "%d%02d%02d-%02d%02d%02d%03d:  %s", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds, logMsg);
                fclose(log);
            }

            dwLastStamp = dwCurrStamp;
            nCountAudioCallBack = 0;
        }
    }
    //end

	unsigned int datalen = 0;
	callback.getCicleBuffer()->readBuffer(pPlayerData, nSize, &datalen);
    //for debug
    if(isSaveDump){
       
        if (outfile1)
        {
            fwrite(this->pPlayerData, 1, datalen, outfile1);
        }
    }

	int nMixLen = nSize;
	if (nSize > 0 && datalen > 0 && audioFrame->buffer)
	{
		int nMixLen = datalen > nSize ? nSize : datalen;
		MixerAddS16((int16_t*)audioFrame->buffer, (int16_t*)pPlayerData, (audioFrame->channels * audioFrame->bytesPerSample) *  audioFrame->samples / sizeof(int16_t));
	}

    //for test
    if (isSaveDump) {
        if (outfile)
        {
            fwrite(audioFrame->buffer, 1, nMixLen, outfile);
        }
    }
	return true;
}

bool CHookPlugin::onPluginPlaybackAudioFrame(AudioPluginFrame* audioFrame)
{
	return true;
}

bool CHookPlugin::onPluginMixedAudioFrame(AudioPluginFrame* audioFrame)
{
    return true;
}

bool CHookPlugin::onPluginPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioPluginFrame* audioFrame)
{
    return true;
}

bool CHookPlugin::onPluginCaptureVideoFrame(VideoPluginFrame* videoFrame)
{
    return true;
}

bool CHookPlugin::onPluginRenderVideoFrame(unsigned int uid, VideoPluginFrame* videoFrame)
{
    return true;
}


int CHookPlugin::load(const char* path)
{
    if (strlen(path) == 0)
        return -1;
    char szFileName[MAX_PATH] = { 0 };
    strncpy_s(szFileName, MAX_PATH, path, strlen(path));
    strcat_s(szFileName, MAX_PATH, "agora_playhook_sdk.dll");

    if (hModule == NULL)
        hModule = LoadLibrary(szFileName);

    if (hModule) {

        createPlayerHookerInstanceFunc = (createPlayerHookerInstanceFuncType*)GetProcAddress(hModule, "createPlayerHookerInstance");
        destoryPlayerHookerInstanceFunc = (destoryPlayerHookerInstanceFuncType*)GetProcAddress(hModule, "destoryPlayerHookerInstance");
        if (createPlayerHookerInstanceFunc) {
            m_lpAgoraPlayerHook = createPlayerHookerInstanceFunc();
        }

        if (m_lpAgoraPlayerHook)
            return 0;
        else
            FreeLibrary(hModule);
    }
    // m_lpAgoraPlayerHook = createPlayerHookerInstance();
    if (m_lpAgoraPlayerHook)
        return 0;
    return -1;
}

int CHookPlugin::unLoad()
{
	if (m_lpAgoraPlayerHook) {
		destoryPlayerHookerInstanceFunc(m_lpAgoraPlayerHook);
		m_lpAgoraPlayerHook = NULL;
	}

	if (hModule) {
		FreeLibrary(hModule);
		hModule = NULL;
	}
	return 0;
}

int CHookPlugin::enable()
{
    if (!m_lpAgoraPlayerHook)
        return -1;

#ifdef UNICODE
    int ret = m_lpAgoraPlayerHook->startHook(musicPlayerPath.c_str(), bForceRestartPlayer);
#else
    WCHAR wsczPath[MAX_PATH] = { 0 };
    MultiByteToWideChar(CP_UTF8, 0, musicPlayerPath.c_str(), MAX_PATH, wsczPath, MAX_PATH);
    int ret = m_lpAgoraPlayerHook->startHook(wsczPath, bForceRestartPlayer);
#endif
    if (ret != 0)
        return -1;

    ret = m_lpAgoraPlayerHook->startAudioCapture(&callback);
    if (ret != 0)
        return -1;

    return 0;
}

int CHookPlugin::disable()
{
	if (!m_lpAgoraPlayerHook) {
		return -1;
	}

	m_lpAgoraPlayerHook->stopHook();
	m_lpAgoraPlayerHook->stopAudioCapture();
	return 0;
}

/*
bool CHookPlugin::setStringParameters(const char* param, const char* value)
{
    std::string strParam = param;
    if (strParam.compare("plugin.hookAudio.playerPath") == 0) {
        musicPlayerPath = value;
    }
    else if (strParam.compare("plugin.hookAudio.hookpath") == 0) {
        hookpath = value;
    }
    return true;
}

bool CHookPlugin::setBoolParameters(const char* param, bool value)
{
 std::string strParam = param;
 if (strParam.compare("plugin.hookAudio.forceRestart") == 0) {
  bForceRestartPlayer = value;
 }
 return true;
}*/

int CHookPlugin::setParameter(const char* param)
{
    Document doc;
    doc.Parse(param);

    if (doc.HasParseError()) {
        return -1;
    }

    if (doc.HasMember("plugin.hookAudio.hookpath")) {
        hookpath = doc["plugin.hookAudio.hookpath"].GetString();
    }

    if (doc.HasMember("plugin.hookAudio.playerPath")) {
        musicPlayerPath = doc["plugin.hookAudio.playerPath"].GetString();
    }

    if (doc.HasMember("plugin.hookAudio.forceRestart")) {
        bForceRestartPlayer = doc["plugin.hookAudio.forceRestart"].GetBool();
    }

    return 0;
}

const char* CHookPlugin::getParameter(const char* key)
{
    if (!key)return NULL;
    if (!strcmp(key, "plugin.hookAudio.forceRestart")) {
        return bForceRestartPlayer ? "true" : "false";
    }
    else if (!strcmp(key, "plugin.hookAudio.playerPath")) {
        return musicPlayerPath.c_str();
    }
    else if (!strcmp(key, "plugin.hookAudio.hookpath")) {
        return hookpath.c_str();
    }
    
    return NULL;
}

int CHookPlugin::release()
{
    delete this;
    return 0;
}

IAVFramePlugin* createAVFramePlugin()
{
    return new CHookPlugin;
}