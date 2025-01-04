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

int main(void)
{
    using namespace httplib;
    Server svr;

    std::cout << "start Server!!" << std::endl;

    // /image でカラーバー画像を返す
    svr.Get("/image", [](const Request& req, Response& res) {
        std::vector<uchar> buffer;  // 画像データを格納するバッファ
        generateColorBar(buffer);   // カラーバー画像を生成してバッファに保存

        // 画像データをHTTPレスポンスに設定
        res.set_content(reinterpret_cast<const char*>(buffer.data()), buffer.size(), "image/jpeg");
    });

    svr.Get("/hi", [](const Request& req, Response& res) {
        res.set_content("Hello World!!!", "text/plain");
    });

    svr.Get(R"(/numbers/(\d+))", [&](const Request& req, Response& res) {
        auto numbers = req.matches[1];
        res.set_content(numbers, "text/plain");
    });

    svr.listen("localhost", 1235);
}