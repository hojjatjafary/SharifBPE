#include <string>
#include <stdexcept> // For std::runtime_error
#include <cstddef>   // For size_t
#include <system_error> // For std::system_error (Windows)

// Platform-specific includes and definitions
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#define NOMINMAX
#include <windows.h>
#else // Assuming Linux/POSIX otherwise
#include <fcntl.h>      // For open()
#include <unistd.h>     // For close()
#include <sys/mman.h>   // For mmap(), munmap()
#include <sys/stat.h>   // For fstat()
#include <cerrno>       // For errno
#include <cstring>      // For strerror
#endif

class MemoryMappedFile 
{
public:
	// Constructor: Opens and maps the file
	MemoryMappedFile(const std::string& filename) 
	{
		openAndMap(filename);
	}

	// Destructor: Unmaps and closes the file
	~MemoryMappedFile() 
	{
		cleanup();
	}

	// Deleted copy constructor and assignment operator to prevent issues
	MemoryMappedFile(const MemoryMappedFile&) = delete;
	MemoryMappedFile& operator=(const MemoryMappedFile&) = delete;

	// Move constructor (optional but good practice)
	MemoryMappedFile(MemoryMappedFile&& other) noexcept
		: m_data(other.m_data)
		, m_size(other.m_size)
#ifdef _WIN32
		, m_hFile(other.m_hFile)
		, m_hMapping(other.m_hMapping)
#else
		, m_fd(other.m_fd)
#endif
	{
		// Nullify the moved-from object's resources
		other.m_data = nullptr;
		other.m_size = 0;
#ifdef _WIN32
		other.m_hFile = INVALID_HANDLE_VALUE;
		other.m_hMapping = NULL;
#else
		other.m_fd = -1;
#endif
	}

	// Move assignment operator (optional but good practice)
	MemoryMappedFile& operator=(MemoryMappedFile&& other) noexcept 
	{
		if (this != &other) 
		{
			// Clean up existing resources first
			cleanup();

			// Transfer ownership
			m_data = other.m_data;
			m_size = other.m_size;
#ifdef _WIN32
			m_hFile = other.m_hFile;
			m_hMapping = other.m_hMapping;
#else
			m_fd = other.m_fd;
#endif

			// Nullify the moved-from object's resources
			other.m_data = nullptr;
			other.m_size = 0;
#ifdef _WIN32
			other.m_hFile = INVALID_HANDLE_VALUE;
			other.m_hMapping = NULL;
#else
			other.m_fd = -1;
#endif
		}
		return *this;
	}

	// Get a pointer to the mapped data
	void* getData() const 
	{
		return m_data;
	}

	// Get the size of the mapped data
	size_t getSize() const 
	{
		return m_size;
	}

	// Check if the mapping is valid
	bool isValid() const 
	{
		return m_data != nullptr;
	}

private:
	void* m_data = nullptr;
	size_t m_size = 0;

#ifdef _WIN32 // Windows specific members
	HANDLE m_hFile = INVALID_HANDLE_VALUE;
	HANDLE m_hMapping = NULL;

	void openAndMap(const std::string& filename) 
	{
		// 1. Open the file
		m_hFile = CreateFileA(
			filename.c_str(),
			GENERIC_READ,       // Read access
			FILE_SHARE_READ,    // Share for reading
			NULL,               // Default security
			OPEN_EXISTING,      // Opens existing file only
			FILE_ATTRIBUTE_NORMAL, // Normal file
			NULL                // No attr. template
		);
		if (m_hFile == INVALID_HANDLE_VALUE) {
			throw std::system_error(GetLastError(), std::system_category(), "Failed to open file: " + filename);
		}

		// 2. Get the file size
		LARGE_INTEGER fileSize;
		if (!GetFileSizeEx(m_hFile, &fileSize)) {
			DWORD error = GetLastError();
			CloseHandle(m_hFile); // Clean up file handle
			m_hFile = INVALID_HANDLE_VALUE;
			throw std::system_error(error, std::system_category(), "Failed to get file size");
		}
		// Check for empty file - mapping might fail or return null
		if (fileSize.QuadPart == 0) {
			CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
			// Decide how to handle: throw, or allow zero-size map (m_data will be null)
			// Let's allow it, isValid() will be false.
			m_size = 0;
			return; // No mapping needed for zero size
		}
		m_size = static_cast<size_t>(fileSize.QuadPart); // Potential truncation on 32-bit for >4GB files

		// 3. Create a file mapping object
		m_hMapping = CreateFileMappingA(
			m_hFile,            // Use file handle
			NULL,               // Default security
			PAGE_READONLY,      // Read access
			0,                  // High-order DWORD of file size (0 for <4GB)
			0,                  // Low-order DWORD of file size (0 means entire file)
			NULL                // Name of mapping object (NULL for unnamed)
		);
		if (m_hMapping == NULL) {
			DWORD error = GetLastError();
			CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
			throw std::system_error(error, std::system_category(), "Failed to create file mapping");
		}

		// 4. Map the view of the file
		m_data = MapViewOfFile(
			m_hMapping,         // Handle to map object
			FILE_MAP_READ,      // Read access
			0,                  // High-order DWORD of offset
			0,                  // Low-order DWORD of offset
			0                   // Map entire file (or size specified in CreateFileMapping)
		);
		if (m_data == nullptr) {
			DWORD error = GetLastError();
			CloseHandle(m_hMapping);
			CloseHandle(m_hFile);
			m_hMapping = NULL;
			m_hFile = INVALID_HANDLE_VALUE;
			throw std::system_error(error, std::system_category(), "Failed to map view of file");
		}

		// Don't close m_hFile here, it's needed by the mapping (though CreateFileMapping docs say it *can* be closed sometimes)
		// CloseHandle(m_hFile); // We'll close it in cleanup
	}

	void cleanup() 
	{
		if (m_data) 
		{
			UnmapViewOfFile(m_data);
			m_data = nullptr;
		}

		if (m_hMapping != NULL) 
		{
			CloseHandle(m_hMapping);
			m_hMapping = NULL;
		}

		if (m_hFile != INVALID_HANDLE_VALUE) 
		{
			CloseHandle(m_hFile);
			m_hFile = INVALID_HANDLE_VALUE;
		}

		m_size = 0;
	}

#else // Linux/POSIX specific members
	int m_fd = -1; // File descriptor

	void openAndMap(const std::string& filename) 
	{
		// 1. Open the file
		m_fd = open(filename.c_str(), O_RDONLY);
		if (m_fd == -1) 
		{
			throw std::runtime_error("Failed to open file '" + filename + "': " + strerror(errno));
		}

		// 2. Get the file size
		struct stat sb;
		if (fstat(m_fd, &sb) == -1) 
		{
			int err = errno;
			close(m_fd); // Clean up file descriptor
			m_fd = -1;
			throw std::runtime_error("Failed to get file size: " + std::string(strerror(err)));
		}

		// Check for empty file
		if (sb.st_size == 0) 
		{
			close(m_fd);
			m_fd = -1;
			// Decide how to handle: throw, or allow zero-size map (m_data will be null)
			// Let's allow it, isValid() will be false.
			m_size = 0;
			return; // No mapping needed for zero size
		}

		m_size = sb.st_size;

		// 3. Map the file into memory
		m_data = (char *)mmap(
			nullptr,            // Let the kernel choose the address
			m_size,             // Length of the mapping
			PROT_READ,          // Read permission
			MAP_SHARED,         // Changes are shared, important for read-only too
			m_fd,               // File descriptor
			0                   // Offset within the file
		);

		if (m_data == MAP_FAILED)
		{
			int err = errno;
			close(m_fd);
			m_fd = -1;
			m_data = nullptr; // Ensure m_data is null on failure
			throw std::runtime_error("Failed to map file to memory: " + std::string(strerror(err)));
		}

		// We can close the file descriptor after mmap succeeds (on POSIX)
		// The mapping keeps a reference to the underlying file description.
		if (close(m_fd) == -1)
		{
			// Non-fatal error, but log it or handle if critical
			// We still need to clean up the mapping later.
			// For robustness, maybe keep fd open until cleanup? Let's close it here.
			// std::cerr << "Warning: closing file descriptor failed after mmap: " << strerror(errno) << std::endl;
		}
		m_fd = -1; // Mark fd as closed
	}

	void cleanup() 
	{
		if (m_data != nullptr)	// Check against nullptr, not MAP_FAILED here
		{
			munmap(m_data, m_size);
			m_data = nullptr;
		}

		if (m_fd != -1) // If fd wasn't closed after mmap for some reason
		{
			close(m_fd);
			m_fd = -1;
		}
		m_size = 0;
	}
#endif
};