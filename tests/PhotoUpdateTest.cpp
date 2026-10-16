#include "PhotoUpdate.h"
#include <fstream>
#include <iostream>
using namespace Gallery;
void require(bool value) {if(!value) throw std::runtime_error("Photo transaction test failed");}
void write(const std::filesystem::path& p,const std::string& s) {std::ofstream(p,std::ios::binary)<<s;}
std::string read(const std::filesystem::path& p) {std::ifstream in(p,std::ios::binary);return {std::istreambuf_iterator<char>(in),{}};}
template<class F> void rejects(F f) {bool rejected=false;try {f();}catch(const std::exception&) {rejected=true;}require(rejected);}
int main() {
 try {
    const auto root=std::filesystem::temp_directory_path()/("OG-photo-update-test-"+std::to_string(GetCurrentProcessId()));
    std::filesystem::create_directories(root/"PhotoRetake");
    const auto target=root/"original.png", a=root/"PhotoRetake"/"a.png", b=root/"PhotoRetake"/"b.png";
    write(target,"old-image"); write(root/"original.json","unchanged equipment"); write(root/"Library.json","unchanged labels and assignments");
    PhotoUpdate update(root);
    for(const auto* name:{"../original.png","sub/original.png","C:/original.png","original.png:stream","original.json","original.png."}) rejects([&]{update.Begin(name);});
    update.Begin("original.png"); write(a,"new-image"); update.Stage(a);
    require(read(target)=="old-image"); update.Cancel(); require(read(target)=="old-image" && !std::filesystem::exists(a));
    update.Begin("original.png"); write(a,"first-take"); update.Stage(a); write(b,"second-take"); update.Stage(b);
    require(!std::filesystem::exists(a) && read(target)=="old-image");
    // Simulate a locked destination: failed commit must preserve both old and draft.
    HANDLE lock=CreateFileW(target.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
    require(lock!=INVALID_HANDLE_VALUE); rejects([&]{update.Commit();}); CloseHandle(lock);
    require(read(target)=="old-image" && read(b)=="second-take");
    update.Commit(); require(read(target)=="second-take"); update.Cancel();
    require(!std::filesystem::exists(b));
    require(read(root/"original.json")=="unchanged equipment" && read(root/"Library.json")=="unchanged labels and assignments");
    update.Begin("missing.png"); write(a,"restored-photo"); update.Stage(a); update.Commit(); update.Cancel();require(read(root/"missing.png")=="restored-photo");
    update.Begin("original.png"); write(a,""); rejects([&]{update.Stage(a);}); rejects([&]{update.Stage(target);}); rejects([&]{update.Commit();});
    std::filesystem::remove_all(root);
    std::cout<<"PASS photo preview/cancel/retake/commit/failed replace/path guards and metadata unchanged\n";
 }catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}
}
