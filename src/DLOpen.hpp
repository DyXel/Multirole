#ifndef DLOPEN_HPP
#define DLOPEN_HPP

namespace DLOpen
{

void* LoadObject(const char* file);
void  UnloadObject(void* handle);

void* LoadFunction(void* handle, const char* name);

} // namespace DLOpen

#endif // DLOPEN_HPP
