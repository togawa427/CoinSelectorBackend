#include "httplib.h"
#include <unistd.h>
#include <iostream>  // 追加
#include <string>
#include <vector>  // 画像データをメモリに保持するために追加

// colorbar.cpp
#include <stdio.h>
// OpenCV用のヘッダファイル
#include <opencv2/opencv.hpp>

//画像ファイル (サイズは小さめが良い)
#define IMG_WIDTH (180) // 画像の幅
#define IMG_HIGHT (100) // 画像の高さ
#define HUE_MAX (180) // Hの最大
#define SAT_MAX (255) // Sの最大
#define VAL_MAX (255) // Vの最大
#define WINDOW_NAME "Hue bar"

// // カラーバーを生成してJPGとして保存する関数
// void generateColorBar(const std::string& filename) {
//     int x, y, h;
//     cv::Vec3b s;

//     // HSV画像、BGR画像領域の確保
//     cv::Mat hsv_img, bar_img;
//     hsv_img = cv::Mat(cv::Size(IMG_WIDTH, IMG_HIGHT), CV_8UC3);

//     // カラーバーの作成
//     // Hの値を変化
//     for (h=0; h<HUE_MAX; h++){
//         s[0] = h;
//         s[1] = SAT_MAX;
//         s[2] = VAL_MAX;
//         for (y=0; y<IMG_HIGHT; y++){
//             hsv_img.at<cv::Vec3b>(y, h) = s;
//         }
//     }

//     // 色変換　HSVからBGR
//     cv::cvtColor(hsv_img, bar_img, cv::COLOR_HSV2BGR);

//     // 画像をJPGとして保存
//     // cv::imwrite(filename, bar_img); // JPGとして保存
//     // メモリバッファに画像をエンコード
//     cv::imencode(".jpg", bar_img, buffer);
// }

// カラーバーを生成してJPGとして返す関数
void generateColorBar(std::vector<uchar>& buffer)
{
    int x, y, h;
    cv::Vec3b s;

    // HSV画像、BGR画像領域の確保
    cv::Mat hsv_img, bar_img;
    hsv_img = cv::Mat(cv::Size(IMG_WIDTH, IMG_HIGHT), CV_8UC3);

    // カラーバーの作成
    // Hの値を変化
    for (h = 0; h < HUE_MAX; h++) {
        s[0] = h;
        s[1] = SAT_MAX;
        s[2] = VAL_MAX;
        for (y = 0; y < IMG_HIGHT; y++) {
            hsv_img.at<cv::Vec3b>(y, h) = s;
        }
    }

    // 色変換　HSVからBGR
    cv::cvtColor(hsv_img, bar_img, cv::COLOR_HSV2BGR);

    // メモリバッファに画像をエンコード
    cv::imencode(".jpg", bar_img, buffer);
}

// 赤要素だけを抽出した画像を生成する関数
// void extractRedChannel(const std::string& srcImpFilename, std::vector<uchar>& buffer) {
//     // 入力画像を読み込む
//     cv::Mat srcImg = cv::imread(srcImpFilename, cv::IMREAD_COLOR);

//     // 画像が読み込めない場合のエラーチェック
//     if (srcImg.empty()) {
//         throw std::runtime_error("Error: Unable to read the input image!");
//     }

//     // 赤要素だけを抽出
//     std::vector<cv::Mat> channels(3);
//     cv::split(srcImg, channels); // BGRチャンネルに分割

//     cv::Mat redChannel = channels[2]; // 赤要素 (BGRの3番目のチャンネル)

//     // 他のチャンネルをゼロにして赤だけの画像を作成
//     cv::Mat redOnlyImg = cv::Mat::zeros(srcImg.size(), srcImg.type());
//     std::vector<cv::Mat> redChannels = {cv::Mat::zeros(srcImg.size(), CV_8UC1), 
//                                          cv::Mat::zeros(srcImg.size(), CV_8UC1), 
//                                          redChannel};
//     cv::merge(redChannels, redOnlyImg);

//     // メモリバッファに赤要素のみの画像を保存
//     cv::imencode(".jpg", redOnlyImg, buffer);
// }

void extractRedChannel(const std::vector<uchar>& inputBuffer, std::vector<uchar>& outputBuffer) {
    // メモリバッファから画像をデコード
    cv::Mat inputImg = cv::imdecode(inputBuffer, cv::IMREAD_COLOR);

    if (inputImg.empty()) {
        throw std::runtime_error("Error: Unable to decode the input image!");
    }

    // BGRチャンネルに分割
    std::vector<cv::Mat> channels(3);
    cv::split(inputImg, channels);

    // 赤チャンネルだけを使用
    cv::Mat redOnlyImg = cv::Mat::zeros(inputImg.size(), inputImg.type());
    std::vector<cv::Mat> redChannels = {cv::Mat::zeros(inputImg.size(), CV_8UC1),
                                         cv::Mat::zeros(inputImg.size(), CV_8UC1),
                                         channels[2]};
    cv::merge(redChannels, redOnlyImg);

    // 赤要素だけの画像をJPEG形式でエンコード
    cv::imencode(".jpg", redOnlyImg, outputBuffer);
}




int main(void)
{
    using namespace httplib;
    Server svr;

    std::cout << "start Server!!" << std::endl;

    // /red で赤要素だけを返す
    // svr.Get("/image", [](const Request& req, Response& res) {
    //     std::string inputFilename = "input.jpg";  // 入力画像ファイル名
    //     std::vector<uchar> buffer;               // 出力バッファ
    //     try {
    //         // 赤要素だけを抽出して画像を生成
    //         extractRedChannel(inputFilename, buffer);

    //         // 生成した赤要素画像をファイルに保存（確認用）
    //         cv::imwrite("red_channel.jpg", cv::imdecode(buffer, cv::IMREAD_COLOR));

    //         std::cout << "Red channel image generated and saved as 'red_channel.jpg'" << std::endl;
    //         // 画像データをHTTPレスポンスに設定
    //         res.set_content(reinterpret_cast<const char*>(buffer.data()), buffer.size(), "image/jpeg");
    //     } catch (const std::exception& e) {
    //         std::cerr << e.what() << std::endl;
    //         res.set_content("fail!!", "text/plain");
    //     }
    // });

    // /imageエンドポイント: POSTで画像を受け取り加工して返す
    svr.Post("/image", [](const Request& req, Response& res) {
        try {
            if (req.has_header("Content-Type") && req.get_header_value("Content-Type") == "image/jpeg") {
                // 受信した画像データ
                std::vector<uchar> inputBuffer(req.body.begin(), req.body.end());

                // 赤要素だけの画像を生成
                std::vector<uchar> outputBuffer;
                extractRedChannel(inputBuffer, outputBuffer);

                // 加工した画像をレスポンスとして返す
                res.set_content(reinterpret_cast<const char*>(outputBuffer.data()), outputBuffer.size(), "image/jpeg");
            } else {
                res.status = 400;  // Bad Request
                res.set_content("Invalid Content-Type. Expected image/jpeg.", "text/plain");
            }
        } catch (const std::exception& e) {
            res.status = 500;  // Internal Server Error
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
        }
    });

    // /image でカラーバー画像を返す
    // svr.Get("/image", [](const Request& req, Response& res) {
    //     std::vector<uchar> buffer;  // 画像データを格納するバッファ
    //     generateColorBar(buffer);   // カラーバー画像を生成してバッファに保存

    //     // 画像データをHTTPレスポンスに設定
    //     res.set_content(reinterpret_cast<const char*>(buffer.data()), buffer.size(), "image/jpeg");
    // });

    svr.Get("/hi", [](const Request& req, Response& res) {
        res.set_content("Hello World!!!", "text/plain");
    });

    svr.Get(R"(/numbers/(\d+))", [&](const Request& req, Response& res) {
        auto numbers = req.matches[1];
        res.set_content(numbers, "text/plain");
    });

    svr.listen("localhost", 1235);
}