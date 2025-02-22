add_library(inicpp INTERFACE)
add_library(inicpp::inicpp ALIAS inicpp)

target_include_directories(inicpp INTERFACE ./)