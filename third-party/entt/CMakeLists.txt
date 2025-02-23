include(FetchContent)

FetchContent_Declare(
  entt
  GIT_REPOSITORY https://github.com/skypjack/entt
  GIT_TAG        4a2d1a8541228a90e02a873dba6a980506c42c03) # 3.14.0
FetchContent_MakeAvailable(entt)

# add this line targets that needs entt
# target_link_libraries(myTarget PRIVATE EnTT::EnTT)