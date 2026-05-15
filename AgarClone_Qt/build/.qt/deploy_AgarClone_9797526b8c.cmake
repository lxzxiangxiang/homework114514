include("D:/code/project/0.0.1/AgarClone_Qt/build/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/AgarClone-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "D:/code/project/0.0.1/AgarClone_Qt/build/AgarClone.exe"
    GENERATE_QT_CONF
)
