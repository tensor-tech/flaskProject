import os

from flask import Flask, request, render_template
from PIL import Image
from io import BytesIO
import time
from imageai.Detection import ObjectDetection
import os

app = Flask(__name__)


@app.route('/')
def hello_world():  # put application's code here
    return 'Hello World!'


@app.route('/imageprocess', methods=['POST'])
def image_preprocess():
    flag = False
    file = request.data
    image = Image.open(BytesIO(file))

    img_path = './static/img/'
    file_name = str(time.strftime('%Y年%m月%d日-%H时%M分%S秒', time.localtime())) + '.jpg'

    execution_path = os.getcwd()

    detector = ObjectDetection()
    detector.setModelTypeAsTinyYOLOv3()
    detector.setModelPath(os.path.join(execution_path, "tiny-yolov3.pt"))
    detector.loadModel()
    detections = detector.detectObjectsFromImage(input_image=image)

    for eachObject in detections:
        print(eachObject["name"], " : ", eachObject["percentage_probability"], " : ", eachObject["box_points"])
        print("--------------------------------")
        if eachObject["name"] == "person":
            flag = True

    if flag:
        image.save(img_path + file_name)

    return 'ok'


@app.route('/getimg', methods=['GET'])
def get_img():
    lst = os.listdir("./static/img/")
    lst.sort()

    return render_template(
        'results.html', results=lst
    )


if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
