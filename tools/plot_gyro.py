import pandas as pd
import matplotlib.pyplot as plt
import glob

# 1) CSV 파일 불러오기 (가장 최근 파일 자동 선택)
files = glob.glob('ds4_report_*.csv')
files.sort()
filename = files[-1]  # 필요 시 직접 경로 지정 가능
print(f"Loading data from: {filename}")

# 2) 데이터 로드
df = pd.read_csv(filename, sep=' ')

# 4) 시간에 따른 자이로 시계열 플롯
plt.figure()
plt.plot(df['time'], df['gyro_x'], label='gyro_x')
plt.plot(df['time'], df['gyro_y'], label='gyro_y')
plt.plot(df['time'], df['gyro_z'], label='gyro_z')
plt.xlabel('Time (s)')
plt.ylabel('Gyro Value')
plt.title('Gyro Sensor Time Series')
plt.legend()
plt.show()

# 5) 시간에 따른 가속도 시계열 플롯
plt.figure()
plt.plot(df['time'], df['acc_x'], label='acc_x')
plt.plot(df['time'], df['acc_y'], label='acc_y')
plt.plot(df['time'], df['acc_z'], label='acc_z')
plt.xlabel('Time (s)')
plt.ylabel('Accel Value')
plt.title('Accelerometer Time Series')
plt.legend()
plt.show()

# 6) 초 단위 샘플링 주파수 계산 및 테이블
df['sec'] = df['time'].astype(int)
freq = df.groupby('sec').size()
freq_df = freq.reset_index()
freq_df.columns = ['second', 'samples_per_sec']

# 7) 샘플링 주파수 플롯
plt.figure()
plt.plot(freq_df['second'], freq_df['samples_per_sec'])
plt.xlabel('Second')
plt.ylabel('Samples per Second (Hz)')
plt.title('Sampling Frequency Over Time')
plt.show()
