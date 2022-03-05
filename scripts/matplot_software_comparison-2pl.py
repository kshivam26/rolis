import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

txt = """
4	37608.26667	7.48E+05	1584450.00
8	65998.83333	1.15E+06	2972510.00
12	86365.53333	1.29E+06	4207410.00
16	95449.9	1.59E+06	5389680.00
20	119235.3	1.76E+06	6573270.00
24	129093.4	1.93E+06	7646680.00
28	137908.6333	2.06E+06	8753250.00
"""

keys, values, values2, values3, values0, values20, values30 = [], [], [], [], [], [], []
idx = 0
for l in txt.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 4:
        continue
    values.append(float(items[1]))
    values2.append(float(items[2]))
    values3.append(float(items[3]))

keys = [4, 8, 12, 16, 20, 24, 28]

fig, ax = plt.subplots(figsize=(14, 9))
ax.set_xticks([4, 8, 12, 16, 20, 24, 28])
ax.set_xticklabels(["4", "", "", "", "", "", "28"])
matplotlib.rcParams['lines.markersize'] = 20
plt.rcParams["font.family"] = "serif"
ax.plot(keys, values, marker="o", label='2PL', linewidth=8)
ax.set_ylim([3.5 * 10000, 14.5 * 10000])
ax.set_xticklabels([])
ax.set_yticklabels([])
ax.yaxis.grid()

#
#
# plt.rcParams["font.size"] = 20
# matplotlib.rcParams['lines.markersize'] = 10
# fig, ax = plt.subplots(nrows=1, ncols=2, figsize=(16, 8))
#
# ax[0].yaxis.set_major_formatter(formatter)
# ax[0].plot(keys, values, marker="s", label='2PL')
# ax[0].plot(keys, values2, marker="*", label='Calvin')
#
# ax[0].set(xlabel='# of partitions or threads',
#           ylabel='Throughput (txns/sec)',
#           title=None)
# # https://stackoverflow.com/questions/4700614/how-to-put-the-legend-out-of-the-plot/43439132#43439132
# ax[0].legend(bbox_to_anchor=(0, 0.92, 1, 0.2), mode="expand", ncol=2, loc="upper left", borderaxespad=0.2, frameon=False)
# ax[0].set_xticks([4, 8, 12, 16, 20, 24, 28])
# ax[0].set_xticklabels(["4", "8", "12", "16", "20", "24", "28"])
# ax[0].yaxis.grid()
# ax[0].set_title("(a) 2PL & Calvin", y=-0.28, fontsize=32)
#
# ax[1].yaxis.set_major_formatter(formatter)
# ax[1].plot(keys, values, marker="o", label='2PL')
# ax[1].plot(keys, values2, marker="s", label='Calvin')
# ax[1].plot(keys, values3, marker="*", label='Rolis')
#
# ax[1].set(xlabel='# of partitions or threads',
#           ylabel='Throughput (txns/sec)',
#           title=None)
# # https://stackoverflow.com/questions/4700614/how-to-put-the-legend-out-of-the-plot/43439132#43439132
# ax[1].legend(bbox_to_anchor=(0, 0.92, 1, 0.2), mode="expand", ncol=3, loc="upper left", borderaxespad=0.2, frameon=False)
# ax[1].set_xticks([4, 8, 12, 16, 20, 24, 28])
# ax[1].set_xticklabels(["4", "8", "12", "16", "20", "24", "28"])
# ax[1].yaxis.grid()
# ax[1].set_title("(b) 2PL & Calvin & Rolis", y=-0.28, fontsize=32)

fig.tight_layout()
fig.savefig("software_comparison-2pl.png", format='png', dpi=1000)
plt.show()