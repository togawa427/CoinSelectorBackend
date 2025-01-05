import requests

# 画像ファイルをPOST
url = "http://localhost:1235/image"
headers = {"Content-Type": "image/jpeg"}
with open("input.jpg", "rb") as f:
    response = requests.post(url, headers=headers, data=f)

# レスポンスの画像を保存
if response.status_code == 200:
    with open("output.jpg", "wb") as f:
        f.write(response.content)
    print("Output image saved as 'output.jpg'")
else:
    print("Error:", response.text)