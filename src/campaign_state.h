#ifndef TYRARGO_CAMPAIGN_STATE_H
#define TYRARGO_CAMPAIGN_STATE_H
#ifdef __cplusplus
extern "C" {
#endif
struct strategyblock;
enum { CAMPAIGN_OFF=0, CAMPAIGN_CHAINED=1, CAMPAIGN_RETAIN=2 };
extern int RetainPlayerState;
int CampaignModeFromPreference(int schema,int value);
void CampaignBeginLevel(void);
void CampaignArmRestart(void);
void CampaignPrepareSavedGame(void);
void CampaignFinishSavedGame(int success);
int CampaignHasEntry(void);
int CampaignEntryPayloadSize(void);
int CampaignExportEntry(void *data,int size);
int CampaignImportEntry(const void *data,int size);
void CampaignCapturePlayer(void);
void CampaignArmNextLevel(void);
void CampaignRestorePlayer(struct strategyblock *player);
#ifdef __cplusplus
}
#endif
#endif
