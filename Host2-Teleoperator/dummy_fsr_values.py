#!/usr/bin/env python3
#  Returns a list of dummy FSR readings with just two FSRs crossing the threshold
import pandas as pd
import random

thresh = 500
num_fsrs = 10

df_fsr_combos = pd.read_csv("array_out_03.csv", usecols=['fsr1', 'fsr2'])

def random_reading(thresh):
    fsr_reading = list((random.randint(0, thresh/5) for i in range(num_fsrs)))
    # A list of length num_fsrs with random values <= threshold/10

    row_num = random.randint(0, len(df_fsr_combos) - 1)  # getting a random row
    # print(row_num, df_fsr_combos.iat[row_num,0], df_fsr_combos.iat[row_num,1])  # printing the values in the random row

    fsr_reading[df_fsr_combos.iat[row_num,0]] = random.randint(thresh + 1, 1024)
    fsr_reading[df_fsr_combos.iat[row_num,1]] = random.randint(thresh + 1, 1024)

    # print(fsr_reading)

    return fsr_reading