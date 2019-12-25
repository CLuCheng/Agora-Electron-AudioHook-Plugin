#pragma once
#include "windows.h"
#include "IAgoraPlayerHook.h"
//#include "IAudioFramePlugin.h"
#include "IAVFramePlugin.h"
#include <string>

//#pragma comment(lib, "agora_playhook_sdk.lib")
typedef IPlayerHooker* (createPlayerHookerInstanceFuncType)();
typedef void (destoryPlayerHookerInstanceFuncType)(IPlayerHooker*);

class CicleBuffer;

class CAudioCaptureCallback : public IAudioCaptureCallback
{
public:
	CAudioCaptureCallback();
	~CAudioCaptureCallback();
	virtual void onCaptureStart() override;
	virtual void onCaptureStop() override;
	virtual void onCapturedData(void* data, UINT dataLen, WAVEFORMATEX* format) override;
	CicleBuffer* getCicleBuffer() { return m_lpHookAudioCicleBuffer; }
private:
	CicleBuffer* m_lpHookAudioCicleBuffer;
};

class CHookPlugin : public IAVFramePlugin
{
public:
 CHookPlugin();
 ~CHookPlugin();

 //unUsed
 virtual bool onPluginCaptureVideoFrame(VideoPluginFrame* videoFrame) override;
 virtual bool onPluginRenderVideoFrame(unsigned int uid, VideoPluginFrame* videoFrame) override;
 
 virtual bool onPluginRecordAudioFrame(AudioPluginFrame* audioFrame) override;
 virtual bool onPluginPlaybackAudioFrame(AudioPluginFrame* audioFrame) override;
 virtual bool onPluginMixedAudioFrame(AudioPluginFrame* audioFrame) override;
 virtual bool onPluginPlaybackAudioFrameBeforeMixing(unsigned int uid, AudioPluginFrame* audioFrame) override;

 virtual bool load(const char* path) override;
 virtual bool unLoad() override;
 virtual bool enable() override;
 virtual bool disable() override;
 virtual bool setParameter(const char* param) override;
 //virtual bool setBoolParameters(const char* param, bool value) override;
 //virtual bool setStringParameters(const char* param, const char* value) override;
 virtual void release() override;
private:
 LPBYTE pPlayerData;
 int    nPlayerDataLen;
 HMODULE hModule;
 IPlayerHooker* m_lpAgoraPlayerHook;
 CAudioCaptureCallback callback;
 createPlayerHookerInstanceFuncType* createPlayerHookerInstanceFunc;
 destoryPlayerHookerInstanceFuncType* destoryPlayerHookerInstanceFunc;
 bool bForceRestartPlayer;
 std::string musicPlayerPath;
 std::string hookpath;
};
