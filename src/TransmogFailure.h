#pragma once
#include <string>
namespace Gallery {
enum class TransmogFailureCode { NotReady, Runtime, Address, Code, Callsite, Jump, Target, Changed };
struct TransmogFailure {
    TransmogFailureCode code{TransmogFailureCode::NotReady};
    bool hair{};
    std::string module;
    std::string detail;
};
inline std::string DescribeTransmogFailure(const TransmogFailure& error,bool japanese) {
    std::string message;
    switch(error.code) {
    case TransmogFailureCode::NotReady: message=japanese?"重ね着の準備がまだ完了していません。":"Transmog is not ready yet.";break;
    case TransmogFailureCode::Runtime: message=japanese?"このゲームバージョンは重ね着の対応対象外です。":"This game version is not supported by transmog.";break;
    case TransmogFailureCode::Address: message=japanese?"重ね着に必要なゲーム処理のアドレスを取得できませんでした。":"Required game addresses for transmog could not be resolved.";break;
    case TransmogFailureCode::Code: message=japanese?"重ね着に必要なゲーム処理の構造を確認できませんでした。":"The game code required for transmog could not be verified.";break;
    case TransmogFailureCode::Callsite: message=japanese?"重ね着の呼出位置を一意に特定できませんでした。":"The transmog call location could not be identified uniquely.";break;
    case TransmogFailureCode::Jump:
        message=japanese?(error.hair?"髪の表示処理":"装備の描画処理"):(error.hair?"Hair display":"Equipment rendering");
        message+=japanese?"が、この版では連携できない方式で変更されています。":" was modified using a hook format this version cannot chain.";
        break;
    case TransmogFailureCode::Target: message=japanese?"重ね着の接続先が読み取り不能、実行不能、または未対応の形式です。":"The transmog target is unreadable, non-executable, or uses an unsupported trampoline.";break;
    case TransmogFailureCode::Changed: message=japanese?"重ね着の準備中に、接続先が別の処理へ変更されました。":"A transmog call changed while initialization was in progress.";break;
    }
    if(!error.module.empty()) message+=(japanese?" 関連DLL: ":" Related DLL: ")+error.module;
    message+=japanese?" 詳細: OutfitGallery.log":" Details: OutfitGallery.log";
    return message;
}
}
