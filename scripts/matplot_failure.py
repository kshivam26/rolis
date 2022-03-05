import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

def millions(x, pos):
    return '%1.1fM' % (x * 1e-6)
formatter = FuncFormatter(millions)

txt = """
213630	307770	506300
207060	344520	587490
214070	334570	590420
217810	344430	588610
211890	338010	581100
213830	334990	599460
215270	304260	595080
215840	336900	581260
217530	360700	597070
219670	357690	612840
222620	363420	598730
220880	353710	620950
226510	359570	610620
215190	357270	635110
218840	366100	620240
219520	352460	638150
216390	364480	626630
219710	364080	630360
216270	366520	630140
217080	368200	632840
222740	366730	633930
224460	364420	629560
223800	356370	636090
222380	346080	629320
217580	362080	633900
221080	367030	613500
219150	368440	647120
219650	372850	631010
219040	364820	632570
214860	362440	619100
216890	359180	628660
224940	362100	627240
218410	356310	615030
221140	358520	641670
217420	376330	617760
217180	352820	631370
216650	351900	634300
218790	364760	641260
218820	357740	639000
220830	365710	621790
221630	372780	626140
219920	359350	637080
218720	365710	634900
217980	362870	631930
219060	355930	639950
219220	358930	630510
221140	372440	634930
219750	364240	635220
215220	361340	627870
220290	361170	631860
216780	351740	628910
216390	369070	637690
221180	364720	615080
216560	359180	621670
211490	366980	631500
218250	362730	633560
217340	367640	630160
216750	366290	607170
220580	368420	628120
218110	364270	635910
220540	365120	636200
215340	380290	634250
217430	366050	631870
215040	360410	631360
216540	351660	631840
217970	354120	646000
214220	361540	638110
215840	365850	634840
215390	365190	613920
212460	358970	636190
216400	367860	630540
221180	363900	628770
216610	362150	608350
218950	353490	650100
211950	358630	631370
215960	363040	640110
215520	347020	616580
215240	344070	626850
222450	352070	636630
215980	359430	638730
216470	352990	627790
215180	350960	620420
214940	358620	622550
212580	356010	630570
215050	359020	630640
213470	356470	631940
219080	362340	630720
214490	366960	623270
214140	366570	635680
213730	364430	625560
216060	363620	620380
212860	366660	618080
220640	370840	625440
215530	362750	600800
216760	357160	608820
216470	350000	615690
216020	365000	609290
215800	363190	625870
219130	363370	629910
218990	362530	618910
0	0	0
0	0	0
0	0	0
0	0	0
0	0	0
0	0	0
0	0	0
0	0	0
0	0	0
0	0	0
0	0	0
0	0	0
0	0	0
0	0	0
0	0	0
0	392070	0
0	404760	0
208100	400250	0
245890	389710	714920
249570	417500	757560
244030	417600	730510
244220	400510	739930
244410	414300	735370
249090	415050	743980
250730	424640	726590
245770	421670	748930
246630	425980	745510
249760	430420	733080
253620	423000	750540
249000	432630	735060
245430	434270	760510
243890	420710	757770
250600	422400	742510
253900	429500	762670
252280	429130	744000
246420	410440	763210
251310	430660	745680
253420	427150	745860
213330	429570	762260
218130	432760	742330
248200	431230	750150
252250	443580	741240
252290	432350	741810
254020	429830	754370
249050	434640	749380
248760	433990	768880
252110	432110	756180
254310	432490	759990
248320	438800	756660
250710	429890	745610
251850	424840	756630
248890	429800	752330
252750	433320	744440
251830	427820	740590
253340	430930	759570
248080	441720	756230
252820	433130	750140
249160	432280	741060
250810	433700	747260
253090	430320	754830
253060	432370	748970
252340	431440	747300
248680	433140	757220
245840	432280	750980
252390	432740	755090
246640	425310	750140
250040	430170	749910
247030	423340	758840
244450	433230	754060
245260	427130	773760
249890	428060	768590
250320	426540	767540
248520	431430	761990
245480	424210	759710
247490	426330	761710
247520	425680	762840
248600	426260	755240
248760	423980	762150
248660	430630	766820
252960	426780	755340
250810	426610	749570
250330	423460	769400
249240	418430	758950
249550	413840	758170
251380	414410	740760
247610	416650	751530
250050	416650	764270
248680	413160	755710
247660	396030	754570
246870	415190	757880
250330	421140	754350
247490	417850	753410
250900	419140	759220
242050	389520	759190
242760	404950	756820
240210	406180	762490
236660	410600	755070
241240	413040	735520
240370	388120	761940
242100	415800	744710
238450	402750	727010
240330	398760	711270
235790	398670	703420
231640	400700	687460
234090	391430	666770
234900	399080	668880
234360	394380	656410
234130	392870	668330
234170	392230	648520
227450	387100	666470
223960	379240	667400
225260	378280	664870
228300	371800	662480
223490	384430	660480
226350	372020	653380
224630	378400	651520
222670	376830	657590
225500	380400	654630
223580	371510	663370
222040	377950	666630
222340	380840	656630
225430	374390	657180
226070	369550	657670
224700	373940	661940
227610	381600	652170
226080	374470	655230
221990	380160	654530
222520	377200	678060
219090	366500	654950
220210	379580	660120
218710	377560	656960
219240	374030	651140
218640	374320	662210
213420	378600	661300
215490	361190	669910
222290	372920	661170
216690	376850	665910
218780	374840	662110
220920	372130	671440
216890	379190	660150
216110	375610	656750
222940	377480	663890
217470	375370	660490
216000	370810	664340
219930	375770	658310
217370	376900	663530
223690	378850	669680
219910	370670	661590
219360	380190	666700
221890	376720	663730
217010	374600	676710
218550	371810	662850
217580	374450	658730
218850	375360	653370
218220	358490	654910
219380	363080	653240
212780	361710	663970
216280	379450	647180
214880	366140	646410
216310	355720	646770
216790	376240	657310
222110	379100	663700
215070	376980	667020
217610	384670	657420
216190	375080	658620
217140	380250	641170
215970	375490	657970
213690	371910	656090
214400	364640	664180
216070	375530	669750
217190	370210	661630
218950	381450	659830
216550	370690	663930
221470	381370	666730
218750	376380	655950
213770	373630	640040
220610	373850	667190
216680	373530	659260
214300	372460	656910
216710	370440	666850
215680	360360	660890
214670	359690	642790
217620	383630	648910
223400	379760	656550
218170	368240	656860
217190	365000	650010
216030	383660	656640
213360	369320	662360
217790	376720	661050
218880	383490	657290
214340	378670	655650
217260	377140	621900
218700	369130	653400
215950	377010	655010
216000	371480	663760
217860	375870	646590
218010	376190	662610
215930	372280	645220
217110	373670	657290
217840	375790	651100
"""
value_0, value_1, value_2, value_3 = [], [], [], []
idx = 0
for l in txt.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 3:
        continue
    idx += 1
    value_0.append(float(items[0]))
    value_1.append(float(items[1]))
    value_2.append(float(items[2]))
    #value_3.append(float(items[3]))

keys_0=[i+1 for i in range(len(value_0))]
keys_1=[i+1 for i in range(len(value_1))]
keys_2=[i+1 for i in range(len(value_2))]
#keys_3=[i+1 for i in range(len(value_3))]

plt.rcParams["font.size"] = 30
matplotlib.rcParams['lines.markersize'] = 14
matplotlib.rcParams['lines.markersize'] = 14
plt.rcParams["font.family"] = "serif"
fig, ax = plt.subplots(figsize=(14, 9))

ax.yaxis.set_major_formatter(formatter)
print(keys_0)
ax.plot(keys_0, value_0, label='4 threads', linewidth=3)
ax.plot(keys_1, value_1, label='8 threads', linewidth=3)
ax.plot(keys_2, value_2, label='16 threads', linewidth=3)
# ax.plot(keys_3, value_3, label='32 cores')
ax.legend(bbox_to_anchor=(0, 0.92, 1, 0.2), mode="expand", ncol=2, loc="upper left", borderaxespad=0.2, frameon=False)
ax.set_xticks(list(range(1, 306, 50)))
ax.set_xticklabels([int(e/10) for e in list(range(0, 306, 50))])

# ax.set(xlabel='Time (sec)',
#        ylabel='Throughput (txns/sec)',
#        title=None)
ax.set_xlabel("Time (sec)", fontname="serif")
ax.set_ylabel("Throughput (txns/sec)", fontname="serif")
ax.legend(bbox_to_anchor=(0, 0.92, 1, 0.2), mode="expand", ncol=4, loc="upper left", borderaxespad=0.2, frameon=False)
ax.grid()
for tick in ax.get_xticklabels():
    tick.set_fontname("serif")
for tick in ax.get_yticklabels():
    tick.set_fontname("serif")
fig.savefig("failure_recovery.eps", format='eps', dpi=1000)
plt.show()