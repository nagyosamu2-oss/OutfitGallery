#pragma once
#include <string>
#include <stdexcept>
#include <utility>
namespace RE {class ExtraDataList;}
namespace Gallery {
struct MissingEnchantmentReference : std::runtime_error {
 std::string reference;
 MissingEnchantmentReference(std::string message,std::string ref):std::runtime_error(std::move(message)),reference(std::move(ref)){}
};
void ValidatePortableEnchantmentReferences(const std::string&);

std::string CapturePortableEnchantment(RE::ExtraDataList*);
void AttachPortableEnchantment(RE::ExtraDataList*,const std::string&);
}
