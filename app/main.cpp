#include "httplib.h"
#include <unistd.h>
#include <iostream>  // 追加
#include <string>
#include <vector>  // 画像データをメモリに保持するために追加
#include "json.hpp"

// colorbar.cpp
#include <stdio.h>
// OpenCV用のヘッダファイル
#include <opencv2/opencv.hpp>
using json = nlohmann::json;

//画像ファイル (サイズは小さめが良い)
#define IMG_WIDTH (180) // 画像の幅
#define IMG_HIGHT (100) // 画像の高さ
#define HUE_MAX (180) // Hの最大
#define SAT_MAX (255) // Sの最大
#define VAL_MAX (255) // Vの最大
#define WINDOW_NAME "Hue bar"

using json = nlohmann::json;

#define FILENAME "IMG_now.JPG" // ok
#define PARAMRATIO (2)
#define MINDIST (20)
#define RANGE (1)

#define MAXRADIUS (50) // 円の最大半径
#define MINRADIUS (20) // 円の最小半径
#define REMOVERANGE (2) // 円の線を消す幅
#define MODV (2) // 大きい円へのアップデートに用いる緩和値
#define IMGWIDTH (680) // 読み込んだ画像のリサイズwidth
#define ITERATION (8) // 円検出の繰り返し数
#define BOXSIZE (3) // ガウシアンウィンドウのサイズ（要:奇数）

// CORS対応用の関数
void set_cors_headers(httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*"); // 必要に応じて "*" を特定のオリジンに変更
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// Base64エンコード用の関数
static const std::string BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

//🐤
void removeCircle(cv::Mat img, std::vector<cv::Vec3f> circles){
    for(int i = 0;i < circles.size(); i++){
        cv::Point pt;
        pt.x = circles[i][0];
        pt.y = circles[i][1];
        int radius = circles[i][2] + REMOVERANGE;
        for(int y = pt.y - radius; y <= pt.y + radius; y++){
            for(int x = pt.x - radius; x <= pt.x + radius; x++){
                if(img.at<uchar>(y,x) == 255){
                    img.at<uchar>(y,x) = 0;
                }
            }
        }
    }
}

// 二つの正円について包含関係があるか判定 
// circle[0:x, 1:y, 2:radius]
bool isImplication(cv::Vec3f circle, cv::Vec3f new_circle){
    if(circle[2] >= new_circle[2]) return(false);
    double distance = sqrt(pow(new_circle[0] - circle[0], 2) + pow(new_circle[1] - circle[1], 2));
    double radiusDiff = abs(new_circle[2] + MODV - circle[2]);
    return distance <= radiusDiff;
}

// 円検出および画像処理を行う関数
// - 引数: 元画像 (src_img)
// - 返り値: 検出されたコイン画像の配列 (coin_imgs) と描画結果画像 (draw_img)
std::pair<std::vector<cv::Mat>, cv::Mat> detectAndProcessCircles(const cv::Mat& src_img) {
    cv::Mat gray_img, edge_img, draw_img = src_img.clone(); // 処理用および描画用の画像を準備
    std::vector<cv::Vec3f> circles, coin_circles; // 円情報を格納するベクトル
    std::vector<cv::Mat> coin_imgs; // 検出された円の切り抜きを格納するベクトル
    cv::Point pt;
    int radius;
    
    // 複数回の円検出処理
    for (int i = 0; i < ITERATION; i++) {
         // グレースケール変換とガウシアンフィルタによるぼかし
        cv::cvtColor(src_img, gray_img, cv::COLOR_BGR2GRAY);
        cv::GaussianBlur(gray_img, gray_img, cv::Size(BOXSIZE + i*2,BOXSIZE + i*2),2,2);
        
        // エッジ検出
        cv::Canny(gray_img, edge_img, 50.0, 140.0);
        // OpenCVの円検出ハフ変換
        circles.clear();
        cv::HoughCircles(edge_img, circles, cv::HOUGH_GRADIENT,
                        PARAMRATIO, MINDIST, 100, 100, MINRADIUS, MAXRADIUS);
        // 包含関係を調べ大きい円に更新
        draw_img = src_img.clone();
        for(int i = 0; i < coin_circles.size(); i++){
            for(int j = 0; j < circles.size(); j++){
                //if(circles[j][2] > MAXRADIUS) continue;
                if(isImplication(coin_circles[i],circles[j])){
                    // std::cout << "update r:" << coin_circles[i][2] << " -> r:" << circles[j][2] << std::endl;
                    pt.x = coin_circles[i][0]; pt.y = coin_circles[i][1]; radius = coin_circles[i][2];
                    cv::circle(draw_img, pt, radius, CV_RGB (0, 0, 255),2,8,0);
                    pt.x = circles[j][0]; pt.y = circles[j][1]; radius = circles[j][2];
                    cv::circle(draw_img, pt, radius, CV_RGB (255, 0, 0),2,8,0);
                    coin_circles[i] = circles[j];
                    cv::Rect rect(pt.x - radius, pt.y - radius, radius * 2, radius * 2);
                    coin_imgs[i] = cv::Mat(src_img, rect);
                }
            }
        }
        
        // すでに検出できた円を取り除き，円検出
        // エッジ検出
        cv::Canny(gray_img, edge_img, 50.0, 140.0);

        // すでに検出できた円をエッジ画像から削除
        removeCircle(edge_img,coin_circles);

        // OpenCVの円検出ハフ変換
        circles.clear();
        cv::HoughCircles(edge_img, circles, cv::HOUGH_GRADIENT,
                        PARAMRATIO, MINDIST, 100, 100, MINRADIUS, MAXRADIUS);
        
        // 検出された円ごとに処理
        for (int i = 0; i < circles.size(); i++) {
            cv::Point pt;
            pt.x = circles[i][0];
            pt.y = circles[i][1];
            radius = circles[i][2];
            
            try{
                cv::Rect rect(pt.x - radius, pt.y - radius, radius * 2, radius * 2);
                coin_imgs.push_back(cv::Mat(src_img, rect));
                coin_circles.push_back(circles[i]);
                // 画像上に円を描画
                cv::circle(draw_img, pt, radius, CV_RGB (0, 255, 0),2,8,0);
            }catch(cv::Exception& e){
                std::cerr << "Error creating rect: " << e.what() << std::endl;
            }
        }
        
        // 処理結果を表示 (デバッグ用)
        // cv::imshow("overlapped image", draw_img);
        // cv::imshow("grayGaussian", gray_img);
        // cv::imshow("edge", edge_img);
        // cv::waitKey(0);
    }

    draw_img = src_img.clone();
    for(int i=0;i<coin_circles.size();i++){
        pt.x = coin_circles[i][0]; pt.y = coin_circles[i][1]; radius = coin_circles[i][2];
        cv::circle(draw_img, pt, radius, CV_RGB (0, 255, 0),2,8,0);
    }
    // 処理結果を表示 (デバッグ用)
    // cv::imshow("overlapped image", draw_img);
    // cv::waitKey(0);
    
    // 検出されたコイン画像と描画画像を返す
    return {coin_imgs, draw_img};
}

//🐤
// スケーリング処理を行う関数
cv::Mat scaleImage(const cv::Mat& src_img, double target_width) {
    cv::Mat scaled_img;
    double scale = target_width / src_img.cols; // 横幅に基づいてスケール計算
    cv::resize(src_img, scaled_img, cv::Size(), scale, scale); // スケーリング実行
    return scaled_img; // スケール済み画像を返す
}

cv::Mat adjustWhiteBalance(const cv::Mat& image) {
    // roiSizeを画像の12%に基づいて計算
    int roiSize = static_cast<int>(std::min(image.cols, image.rows) * 0.12);
    // 四隅の白い紙の領域を基準としてホワイトバランス補正
    std::vector<cv::Rect> cornerROIs = {
        cv::Rect(0, 0, roiSize, roiSize),                                     // 左上
        cv::Rect(image.cols - roiSize, 0, roiSize, roiSize),                 // 右上
        cv::Rect(0, image.rows - roiSize, roiSize, roiSize),                 // 左下
        cv::Rect(image.cols - roiSize, image.rows - roiSize, roiSize, roiSize) // 右下
    };
    
    // デバッグ: 四隅領域を描画
    cv::Mat debugImage = image.clone();
    for (const auto& roi : cornerROIs) {
        cv::rectangle(debugImage, roi, cv::Scalar(0, 255, 0), 1); // 緑の枠を描画
    }
    
    // 各コーナーの平均色を計算
    cv::Scalar paperColor(0, 0, 0);
    for (const auto& roi : cornerROIs) {
        cv::Mat cornerROI = image(roi);
        paperColor += cv::mean(cornerROI);
    }
    
    // 平均値を取得
    paperColor[0] /= cornerROIs.size();  // Blue チャンネル
    paperColor[1] /= cornerROIs.size();  // Green チャンネル
    paperColor[2] /= cornerROIs.size();  // Red チャンネル
    
    // ホワイトバランス補正係数を計算
    double blueGain = 255.0 / paperColor[0];
    double greenGain = 255.0 / paperColor[1];
    double redGain = 255.0 / paperColor[2];
    
    // ホワイトバランス補正を適用
    std::vector<cv::Mat> channels(3);
    cv::split(image, channels);
    // 各チャンネルの補正後、値を255でクリップ
    channels[0] = cv::min(channels[0] * blueGain, 255.0);  // Blue
    channels[1] = cv::min(channels[1] * greenGain, 255.0); // Green
    channels[2] = cv::min(channels[2] * redGain, 255.0);   // Red
    cv::merge(channels, image);
    
    return image;

}

//明るさ正規化
cv::Mat normalizeBrightness(const cv::Mat& image) {
    // HSV空間で明るさ（Vチャンネル）の均等化
    cv::Mat hsvImage;
    cv::cvtColor(image, hsvImage, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> hsvChannels(3);
    cv::split(hsvImage, hsvChannels);
    
    // Vチャンネルのヒストグラム均等化
    cv::equalizeHist(hsvChannels[2], hsvChannels[2]);
    
    // チャンネルをマージして元のBGR空間に変換
    cv::merge(hsvChannels, hsvImage);
    cv::Mat normalizedImage;
    cv::cvtColor(hsvImage, normalizedImage, cv::COLOR_HSV2BGR);
    
    return normalizedImage;
}

// マスク作成
cv::Mat createMask_easy(const cv::Mat& image) {
    int shortSide = std::min(image.cols, image.rows);
    cv::Point center2(image.cols / 2, image.rows / 2);
    int radius2 = static_cast<int>(shortSide * 0.5);
    cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
    cv::circle(mask, center2, static_cast<int>(radius2-6), cv::Scalar(255), -1); // 円を描画
    return mask;
}


// 色情報の計算 (硬貨部分の平均色を取得 - HSVで出力)
cv::Scalar calculateMeanColorHSV(const cv::Mat& image, const cv::Mat& mask) {
    // BGRからHSVに変換
    cv::Mat hsvImage;
    cv::cvtColor(image, hsvImage, cv::COLOR_BGR2HSV);
    
    // HSVでの平均色を計算
    return cv::mean(hsvImage, mask); // マスクで指定された部分の平均色を計算 (HSV形式)
}

//穴の有無
//あったらtrue,なかったらfalse
bool holeFlg(const cv::Mat& image) {
    if (image.empty()) {
        std::cerr << "画像が空です。" << std::endl;
        return false;
    }
    
    // 中心点を計算
    int centerX = image.cols / 2;
    int centerY = image.rows / 2;
    
    // 判定用矩形のサイズを計算（画像サイズの5%）
    int roiSize = static_cast<int>(std::min(image.cols, image.rows) * 0.05);
    
    // 矩形の左上座標
    int startX = std::max(centerX - roiSize / 2, 0);
    int startY = std::max(centerY - roiSize / 2, 0);
    
    // 矩形の右下座標が画像サイズを超えないように調整
    int width = std::min(roiSize, image.cols - startX);
    int height = std::min(roiSize, image.rows - startY);
    
    // 中心領域をROIとして取得
    cv::Rect centerROI(startX, startY, width, height);
    cv::Mat roi = image(centerROI);
    
    // ROIの平均色を計算
    cv::Scalar meanColor = cv::mean(roi);

    // std::cout << "B: " << meanColor[0] << ", G: " << meanColor[1] << ", R: " << meanColor[2]<< std::endl;
    
    // 判定条件: RGB全てが200以上であれば白
    if (meanColor[0] >= 220 && meanColor[1] >= 220 && meanColor[2] >= 220) {
        return true; // 穴がある（中心が白い）
    }
    return false; // 穴がない
}

//硬貨分類(HSV)
int classifyCoinHSV(const cv::Mat& image, const cv::Scalar& meanHSV) {
    
    //デバッグ用
//    std::cout << "H:" << meanHSV[0] << ", S:"<< meanHSV[1] <<", V:" <<meanHSV[2] <<  std::endl;
//    cv::imshow("判定中", image);
//    cv::waitKey(0);
    
    if(80<=meanHSV[1]){//彩度が高い場合
        if(holeFlg(image)){//穴がある場合
//            std::cout << "5円" << std::endl;
            return 5;
        }else{//穴がない場合
//            std::cout << "10円" << std::endl;
            return 10;
        }
    } else if(25<=meanHSV[1]){//彩度が中程度の場合
        if(holeFlg(image)){//穴がある場合
//            std::cout << "50円" << std::endl;
            return 50;
        }else{//穴がない場合
//            std::cout << "100円" << std::endl;
            return 100;
        }
    }else{//彩度が低い場合
        return 1;
    }
    
}

//ここで全ての関数を呼び出していく
std::tuple<cv::Mat, int, int, int, int, int, int> detectCoin(const cv::Mat& inputImage) {
    cv::Mat image = inputImage.clone();
    
    //🐤
    // スケール処理を関数で実行
    cv::Mat scaled_img = scaleImage(image, IMGWIDTH);
    
    //🐤
    // 円検出および描画
    std::pair<std::vector<cv::Mat>, cv::Mat> result = detectAndProcessCircles(scaled_img);
    std::vector<cv::Mat> coin_imgs = result.first;//コインの画像集
    cv::Mat draw_img = result.second;//輪郭が緑の全体の画像
    
    int sum =0;
    int coin_1=0;
    int coin_5=0;
    int coin_10=0;
    int coin_50=0;
    int coin_100=0;
    
    //各コインを判別
    for (size_t i = 0; i < coin_imgs.size(); i++) {
        image = coin_imgs[i];
        // ホワイトバランス調整
        image = adjustWhiteBalance(image);
        
        // 明るさ正規化
        // image = normalizeBrightness(image);
    
        // マスク作成
        cv::Mat mask = createMask_easy(image);
        
        // HSVの平均色を計算
        cv::Scalar meanHSV = calculateMeanColorHSV(image, mask);
        
        int kinds = classifyCoinHSV(image, meanHSV);
        if(kinds == 1)coin_1 = coin_1 + 1;
        if(kinds == 5)coin_5 = coin_5 + 1;
        if(kinds == 10)coin_10 = coin_10 + 1;
        if(kinds == 50)coin_50 = coin_50 + 1;
        if(kinds == 100)coin_100 = coin_100 + 1;
        sum = sum + kinds;
    }

    // 複数の値を返す
    return std::make_tuple(draw_img, coin_1, coin_5, coin_10, coin_50, coin_100, sum);
}

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

    // 出力画像を保存
    if (!cv::imwrite("output.jpg", redOnlyImg)) {
        throw std::runtime_error("Error: Unable to save the output image!");
    }
}

// コインの輪郭を検出し、その数を数える関数
std::map<int, int> countCoins(const std::vector<uchar>& inputBuffer, std::vector<uchar>& outputBuffer) {

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

    // 出力画像を保存
    if (!cv::imwrite("output.jpg", redOnlyImg)) {
        throw std::runtime_error("Error: Unable to save the output image!");
    }

    // コインの半径に基づいて分類（仮の値）
    std::map<int, int> coinCounts = {
        {100, 3}, // 100円玉
        {50, 4},  // 50円玉
        {10, 5},  // 10円玉
        {5, 6},   // 5円玉
        {1, 8}    // 1円玉
    };

    return coinCounts;
}

// 画像をBase64に変換する関数
std::string vectorToBase64(const std::vector<uchar>& data) {
    std::string encoded_string;
    unsigned char const* bytes_to_encode = data.data();
    size_t in_len = data.size();
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; (i < 4); i++) {
                encoded_string += BASE64_CHARS[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 3; j++) {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++) {
            encoded_string += BASE64_CHARS[char_array_4[j]];
        }

        while ((i++ < 3)) {
            encoded_string += '=';
        }
    }

    return encoded_string;
}


// cv::Mat を Base64 に変換する関数
std::string matToBase64(const cv::Mat& img, const std::string& format = ".jpg") {
    std::vector<uchar> buffer;
    // 画像をバイナリデータにエンコード
    if (!cv::imencode(format, img, buffer)) {
        throw std::runtime_error("Failed to encode image to buffer");
    }
    // バイナリデータを Base64 にエンコード
    return vectorToBase64(buffer);
}



int main(void)
{
    using namespace httplib;
    Server svr;

    std::cout << "start Server!!" << std::endl;

    // CORS用のプリフライトリクエスト（OPTIONSリクエスト）対応
    svr.Options("/image", [](const httplib::Request &req, httplib::Response &res) {
        set_cors_headers(res);
        res.status = 200; // OK
    });
    svr.Options("/imagePlus", [](const httplib::Request &req, httplib::Response &res) {
        set_cors_headers(res);
        res.status = 200; // OK
    });
    svr.Options("/hi", [](const httplib::Request &req, httplib::Response &res) {
        set_cors_headers(res);
        res.status = 200; // OK
    });

    // /imageエンドポイント: POSTで画像を受け取り加工して返す
    svr.Post("/imagePlus", [](const Request& req, Response& res) {
        std::cout << "recieve request!!" << std::endl;
        try {
            set_cors_headers(res); // CORSヘッダーをレスポンスに追加
            if (req.has_header("Content-Type") && req.get_header_value("Content-Type") == "image/jpeg") {
                // 受信した画像データ
                std::vector<uchar> inputBuffer(req.body.begin(), req.body.end());
                cv::Mat inputImg = cv::imdecode(inputBuffer, cv::IMREAD_COLOR);

                std::cout << "start!\n";

                // 赤要素だけの画像を生成
                // std::vector<uchar> outputBuffer;
                // extractRedChannel(inputBuffer, outputBuffer);
                // std::map<int, int> coinCounts = countCoins(inputBuffer, outputBuffer);

                //変数用意
                int coin_1, coin_5, coin_10, coin_50, coin_100, sum;
                cv::Mat result_img;

                std::cout << "totyuu1\n";
                
                // 硬貨判定
                std::tie(result_img, coin_1, coin_5, coin_10, coin_50, coin_100, sum) = detectCoin(inputImg);

                std::cout << "totyuu2\n";
                
                //結果出力
                std::cout << "1円: " << coin_1 << ", 5円: " << coin_5 << ", 10円: " << coin_10
                << ", 50円: " << coin_50 << ", 100円: " << coin_100 << ", 合計: " << sum << std::endl;
                // 加工画像をBase64に変換
                // std::string processedImgBase64 = vectorToBase64(result_img);
                std::string processedImgBase64 = matToBase64(result_img);
                

                json response = {
                    {"hundred_yen", coin_100},
                    {"fifty_yen", coin_50},
                    {"ten_yen", coin_10},
                    {"five_yen", coin_5},
                    {"one_yen", coin_1},
                    {"processedImage", processedImgBase64}
                };

                // 加工した画像をレスポンスとして返す
                res.set_content(response.dump(), "application/json");
                // res.set_content(reinterpret_cast<const char*>(outputBuffer.data()), outputBuffer.size(), "image/jpeg");
                std::cout << "success!" << std::endl;
            } else {
                res.status = 400;  // Bad Request
                res.set_content("Invalid Content-Type. Expected image/jpeg.", "text/plain");
            }
        } catch (const std::exception& e) {
            res.status = 500;  // Internal Server Error
            res.set_content(std::string("Error: ") + e.what(), "text/plain");
        }
    });

    // /imageエンドポイント: POSTで画像を受け取り加工して返す
    svr.Post("/image", [](const Request& req, Response& res) {
        std::cout << "recieve request!!" << std::endl;
        try {
            set_cors_headers(res); // CORSヘッダーをレスポンスに追加
            if (req.has_header("Content-Type") && req.get_header_value("Content-Type") == "image/jpeg") {
                // 受信した画像データ
                std::vector<uchar> inputBuffer(req.body.begin(), req.body.end());

                // 赤要素だけの画像を生成
                std::vector<uchar> outputBuffer;
                extractRedChannel(inputBuffer, outputBuffer);

                // 加工した画像をレスポンスとして返す
                res.set_content(reinterpret_cast<const char*>(outputBuffer.data()), outputBuffer.size(), "image/jpeg");
                std::cout << "success!" << std::endl;
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

    svr.listen("0.0.0.0", 3000);
}