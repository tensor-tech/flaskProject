import glob
import os

import schedule
from aligo import Aligo
import time


def job():
    ali = Aligo()  # 第一次使用，会弹出二维码，供扫描登录
    esp32_cam_file_id = ali.get_folder_by_path("esp32_cam", create_folder=True).file_id
    today = time.strftime('%Y年%m月%d日', time.localtime())
    today_file_id = ali.get_folder_by_path(today, parent_file_id=esp32_cam_file_id,create_folder=True).file_id

    ali.sync_folder(local_folder="/home/lighthouse/esp32cam/static/img/", remote_folder=today_file_id)

    for file in glob.glob("/home/lighthouse/esp32cam/static/img/*"):
        os.remove(file)
        print("Deleted " + str(file))


if __name__ == '__main__':
    schedule.every().hour.do(job)

    while True:
        schedule.run_pending()
        time.sleep(1)



