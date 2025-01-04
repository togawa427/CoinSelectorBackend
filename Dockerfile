FROM ubuntu:22.04

RUN apt update
RUN apt install -y make
RUN apt install -y cmake
RUN apt install -y g++
# RUN apt install -y libopencv-dev # 手動でインストールしよう

# COPYでホストのファイルをコンテナ内にコピー
COPY /app /coinselector/

# CMD ["bash"]