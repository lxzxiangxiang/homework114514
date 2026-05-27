include("D:/code/project/0.0.1/AgarClone_Qt/build/Desktop_Qt_6_11_0_MinGW_64_bit-Profile/.qt/QtDeploySupport.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/AgarClone-plugins.cmake" OPTIONAL)
set(__QT_DEPLOY_I18N_CATALOGS "qtbase")

qt6_deploy_runtime_dependencies(
    EXECUTABLE "D:/code/project/0.0.1/AgarClone_Qt/build/Desktop_Qt_6_11_0_MinGW_64_bit-Profile/AgarClone.exe"
    GENERATE_QT_CONF
)
