cmake_minimum_required(VERSION 3.25)
project(QuickSplitTests LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Test)
# This small harness never initializes the VPN service or real user settings.
add_executable(quick_split_preview preview.cpp)
target_link_libraries(quick_split_preview PRIVATE Qt6::Gui Qt6::Quick Qt6::QuickControls2)
target_compile_definitions(quick_split_preview PRIVATE QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../ui/qml")
enable_testing()
add_executable(test_quick_split testQuickSplit.cpp
    ../../ui/controllers/quickSplitController.cpp
    ../../ui/controllers/quickSplitController.h)
target_link_libraries(test_quick_split PRIVATE Qt6::Core Qt6::Test)
add_test(NAME QuickSplit COMMAND test_quick_split)
add_executable(test_quick_split_ui testQuickSplitUi.cpp)
target_link_libraries(test_quick_split_ui PRIVATE Qt6::Test Qt6::Quick Qt6::QuickControls2)
target_compile_definitions(test_quick_split_ui PRIVATE QML_DIR="${CMAKE_CURRENT_SOURCE_DIR}/../../ui/qml")
add_test(NAME QuickSplitUi COMMAND test_quick_split_ui)
add_executable(test_connection_notifications testConnectionNotifications.cpp
    ../../ui/utils/connectionNotificationGate.cpp
    ../../ui/utils/connectionNotificationGate.h
    ../../ui/controllers/quickSplitController.cpp
    ../../ui/controllers/quickSplitController.h)
target_link_libraries(test_connection_notifications PRIVATE Qt6::Core Qt6::Test)
add_test(NAME ConnectionNotifications COMMAND test_connection_notifications)
