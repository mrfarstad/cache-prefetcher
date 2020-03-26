import matplotlib
from matplotlib import pyplot as plt

stop_words = [
    "swim",
    "ammp",
    "twolf",
    "bzip2_source",
    "bzip2_program",
    "bzip2_graphic",
    "art110",
    "art470",
    "wupwise",
    "appsi",
    "applu",
    "galgel",
]


def harmonic_mean(speedups):
    mean = 0
    for speedup in speedups:
        mean += 1 / speedup
    return len(speedups) / mean


path = "width_degree_3"

means = []

with open(path + "/ghb_ait_256/stats.txt", "r") as file:
    lines = file.readlines()
    for i in range(0, len(lines)):
        if "PREFETCHER: user" in lines[i]:
            # print(lines[i:])
            user_stats = lines[i:]
            break

    user_results = {}

    for line in user_stats:
        for word in stop_words:
            if word in line:
                test_results = line.split()
                user_results[test_results[0]] = float(test_results[2])
                print(test_results[0], user_results[test_results[0]])
# Is this
# plt.bar(list(user_results.keys()), list(user_results.values()))
plt.ylim([0.95, 1.15])
plt.ylabel("Speedup")
plt.bar(list(reversed(list(user_results.keys()))),
        list(reversed(list(user_results.values()))))
plt.yticks(list(user_results.values()))
plt.xticks(rotation='vertical')
plt.tight_layout()
plt.savefig("size_256")
plt.show()
