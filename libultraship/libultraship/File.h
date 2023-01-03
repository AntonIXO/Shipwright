#pragma once

#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace Ship {
	class Archive;

	class File
	{
	public:
		std::shared_ptr<Archive> parent;
		std::string path;
		std::shared_ptr<char[]> buffer;
		uint32_t dwBufferSize;
		bool bIsLoaded = false;
		bool bHasLoadError = false;
		std::condition_variable* FileLoadNotifier;
		std::mutex* FileLoadMutex;

		File() {
			FileLoadNotifier = new std::condition_variable();
			FileLoadMutex = new std::mutex();
		}

		void releaseSyncObjects() {
			if (FileLoadNotifier) {
				delete FileLoadNotifier;
				FileLoadNotifier = nullptr;
			}
			
			if (FileLoadMutex) {
				delete FileLoadMutex;
				FileLoadMutex = nullptr;
			}
		}

		~File() {
			if (FileLoadNotifier)
				delete FileLoadNotifier;
			if (FileLoadMutex)
				delete FileLoadMutex;
		}
	};
}
