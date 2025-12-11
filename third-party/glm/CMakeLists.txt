include(FetchContent)

FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	a532f5b1cf27d6a3c099437e6959cf7e398a0a67 #refs/tags/1.0.2
)

FetchContent_MakeAvailable(glm)

# add this line targets that needs glm
# target_link_libraries(myTarget PRIVATE glm::glm)
