del /F /A *.xml
copy ..\..\..\..\..\reswork\boot\config\ServerInfo.xml

del /F /A 6.scope
del /F /A 7.scope
del /F /A 21.scope
del /F /A 22.scope
del /F /A 23.scope
del /F /A 24.scope
del /F /A 25.scope
del /F /A 1006.scope
del /F /A 1007.scope
del /F /A 1021.scope
del /F /A 1022.scope
del /F /A 1023.scope
del /F /A 1024.scope
del /F /A 2000.scope
del /F /A 2001.scope
del /F /A 2002.scope
del /F /A 2003.scope
copy ..\..\..\..\..\reswork\scene\TG_GEY\TG_GEY.scope 6.scope
copy ..\..\..\..\..\reswork\scene\TG_GEYYJ\TG_GEYYJ.scope 7.scope
copy ..\..\..\..\..\reswork\scene\TG_Ruyiyuan\TG_Ruyiyuan.scope 21.scope
copy ..\..\..\..\..\reswork\scene\TG_yonghuayuan\TG_yonghuayuan.scope 22.scope
copy ..\..\..\..\..\reswork\scene\TG-JXY\TG-JXY.scope 23.scope
copy ..\..\..\..\..\reswork\scene\TG_Fuguiyuanxin\TG_Fuguiyuanxin.scope 24.scope
copy ..\..\..\..\..\reswork\scene\TG_Zengfuyuan\TG_Zengfuyuan.scope 25.scope
copy ..\..\..\..\..\UnityProject\CienTianxia\Assets\Scenes\CE_GEY\CE_GEY\CE_GEY.scope 1006.scope
copy ..\..\..\..\..\UnityProject\CienTianxia\Assets\Scenes\CE_GEYYJ\CE_GEYYJ\CE_GEYYJ.scope 1007.scope
copy ..\..\..\..\..\UnityProject\CienTianxia\Assets\Scenes\CE_RYY\CE_RYY\CE_RYY.scope 1021.scope
copy ..\..\..\..\..\UnityProject\CienTianxia\Assets\Scenes\CE_YHY\CE_YHY\CE_YHY.scope 1022.scope
copy ..\..\..\..\..\UnityProject\CienTianxia\Assets\Scenes\CE_JXY\CE_JXY\CE_JXY.scope 1023.scope
copy ..\..\..\..\..\UnityProject\CienTianxia\Assets\Scenes\CE_FGY\CE_FGY\CE_fgy.scope 1024.scope
copy ..\..\..\..\..\reswork\scene\TG_Fojiaoqu\TG_Fojiaoqu.scope 2000.scope
copy ..\..\..\..\..\reswork\scene\TG_Gonggongqu01\TG_Gonggongqu01.scope 2001.scope
copy ..\..\..\..\..\reswork\scene\TG_tdys\TG_tdys.scope 2002.scope
copy ..\..\..\..\..\reswork\scene\TG_Tdys_huangdi\TG_Tdys_huangdi.scope 2003.scope

copy ..\..\..\..\..\reswork\boot\config\ServerInfo.xml ..\..\12_manager_service\

copy ..\OutData.ini ..\..\12_manager_service\
copy ..\ServerLang.ini ..\..\10_history_service\
copy ..\ServerLang.xml ..\..\10_history_service\
copy ..\ServerLang.ini ..\..\12_manager_service\
copy ..\ServerLang.xml ..\..\12_manager_service\
pause
