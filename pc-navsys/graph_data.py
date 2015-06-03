from __future__ import unicode_literals

import json, pprint, time

from matplotlib import rc
font = {'family': 'Times New Roman',
        'weight': 'normal',
        'size': '15'}
rc('font', **font)



import matplotlib.pyplot as plt

def tosec(jtime):
    return ((jtime["h"] * 24) + (jtime["m"] * 60) +  jtime["s"])


# Массивы данных для рисования
mplTimeData = []
mplNumsatData = []
mplHdopData = []
mplHeightData = []
mplLatData = []
mplLonData = []


# JSON-файл для рисования
file = "E:\Dropbox\Dropbox\ee_logs\out_antenna_outside_window"

with open(file) as hugeJSON:
    # грузим JSON
    d = json.load(hugeJSON)

    startsec = 0

    # для каждой записи о навигации
    for member in d["navdatas"]:
        
        navdata = member['navdata']
        
        jtime = navdata['time']

        # подсчитываем секунды приёмника (с начала дня)
        totsec = tosec(jtime)

        
        if (totsec == 0):
            continue
        else:
            if (startsec == 0):
                # Момент первого сигнала от спутника, пишем данные с него
                startsec = totsec

                
            # когда остановиться   
            if (totsec - startsec == 250):
                break
            

            # сохранияем данные из JSON в массивы
            mplTimeData.append(totsec - startsec)
            print(navdata['numsat'], totsec - startsec)
            mplNumsatData.append(navdata["numsat"])

            mplHdopData.append(navdata["hdop"])
            mplHeightData.append(navdata["height"])

            mplLatData.append(navdata["lat"])
            mplLonData.append(navdata["lon"])               

#pprint.pprint(mplTimeData)

mplDLatData = []

# высчитываем "шум" в младших разрядах градусов
for i in range(0, len(mplLatData)):
    delta = abs(mplLatData[i] - mplLatData[i - 1])
    if (delta < 1.0):
        mplDLatData.append(delta)
    else:
        mplDLatData.append(0.0)


mplDLonData = []

for i in range(0, len(mplLonData)):
    delta = abs(mplLonData[i] - mplLonData[i - 1])
    if (delta < 0.0001):
        mplDLonData.append(delta)
    else:
        mplDLonData.append(0.0)


# строим графики

#pprint.pprint(mplNumsatData)


plt.subplot(511)
#plt.xlabel(u"Время, с")
#plt.ylabel(u"К-во спутников")

plt.plot(mplTimeData, mplNumsatData, '-')


plt.subplot(512)

#plt.xlabel(u"Время, с")
#plt.ylabel(u"Горизонтальная погрешность, м")

plt.plot(mplTimeData, mplHdopData, '-')

plt.subplot(513)

#plt.xlabel(u"Время, с")
#plt.ylabel(u"Высота антенны, м")

plt.plot(mplTimeData, mplHeightData, '-')

plt.ylim([125,155]) # Отдельные пределы для высоты

plt.subplot(514)

#plt.xlabel(u"Время, с")
#plt.ylabel(u"Нестаб. шир. стацион. ант., град.")

plt.plot(mplTimeData, mplDLatData, '-')

plt.subplot(515)

#plt.xlabel(u"Время, с")
#plt.ylabel(u"Нестаб. дол. стацион. ант., град.")

plt.plot(mplTimeData, mplDLonData, '-')

plt.show()
            
        
        
        
    
    







