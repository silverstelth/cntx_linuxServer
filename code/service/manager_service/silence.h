#ifndef SILENCE_H
#define SILENCE_H

extern void LoadSilenceList();
extern void cb_M_GMCS_SILENCE_SET(SIPNET::CMessage &_msgin, const std::string &_serviceName, SIPNET::TServiceId _tsid);

extern void SendSilenceListToFS(SIPNET::TServiceId _tsid);

#endif // SILENCE_H
