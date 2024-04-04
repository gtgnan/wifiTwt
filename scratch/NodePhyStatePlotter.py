import matplotlib.pyplot as plt
import numpy as np

# Read the log.statelog file
with open('log.statelog', 'r') as file:
    lines = file.readlines()

# Parse through the log file and extract relevant information
stations = {}
for line in lines:
    parts = line.strip().split(';')
    state = parts[0].split('=')[1]
    startTime = int(parts[1].split('=')[1])
    endTime = int(parts[3].split('=')[1])
    station_id = int(parts[4].split('=')[1])
    
    if station_id not in stations:
        stations[station_id] = {'times': [], 'states': []}
    
    stations[station_id]['times'].append(startTime)
    stations[station_id]['states'].append(state)
    stations[station_id]['times'].append(endTime)
    stations[station_id]['states'].append(state)

# Specify the timeframe to plot
start_time_ns = 100*102.4*1e6 # 100 beacon intervals - roughly 10.2 seconds
plot_start_time_ns = 100.9*102.4*1e6 
end_time_ns = 104*102.4*1e6
plot_end_time_ns = 103.1*102.4*1e6

beaconInterval_us = 102400

# Create the plot with subplots
n = len(stations)
fig, axs = plt.subplots(n, 1, sharex=True, figsize=(10, 5*n))

# Iterate over each station and plot the data
# Sort the stations by their ID
stations = dict(sorted(stations.items()))
for i, (station_id, data) in enumerate(stations.items()):
    times = data['times']
    states = data['states']
    
    # Filter the data based on the specified timeframe
    filtered_times = [t for t in times if start_time_ns <= t <= end_time_ns]
    filtered_states = [0 if s == 'SLEEP' else 1 for s, t in zip(states, times) if start_time_ns <= t <= end_time_ns]
    filtered_times_s = [t/1e9 for t in filtered_times]
    # Plot the data in the corresponding subplot
    if isinstance(axs, np.ndarray):
        axs[i].plot(filtered_times_s, filtered_states)
        axs[i].set_ylabel(f'STA {station_id}')
        axs[i].yaxis.label.set_rotation(0)
        axs[i].set_ylim([-0.5, 1.5])
        axs[i].set_xlim([plot_start_time_ns/1e9, plot_end_time_ns/1e9])
    else:
        axs.plot(filtered_times_s, filtered_states)
        axs.set_ylabel(f'STA {station_id}')
        axs.yaxis.label.set_rotation(0)
        axs.set_ylim([-0.5, 1.5])
        axs.set_xlim([plot_start_time_ns/1e9, plot_end_time_ns/1e9])

# For all subplots, add a vertical line at points where time is a multiple of 102.4ms
((start_time_ns/1000)//beaconInterval_us)*beaconInterval_us/1e6
if isinstance(axs, np.ndarray):
    for ax in axs:
        for i in np.arange(((start_time_ns/1000)//beaconInterval_us)*beaconInterval_us/1e6, end_time_ns/1e9, 0.1024):
            ax.axvline(x=i, color='gray', linestyle='--', linewidth=0.5)
else:
    for i in np.arange(((start_time_ns/1000)//beaconInterval_us)*beaconInterval_us/1e6, end_time_ns/1e9, 0.1024):
        axs.axvline(x=i, color='gray', linestyle='--', linewidth=0.5)

# Set the x-axis label and tick labels
if isinstance(axs, np.ndarray):
    axs[-1].set_xlabel('Time (s)')
else:
    axs.set_xlabel('Time (s)')
# plt.xticks(rotation=45)
# Add padding below the x-axis ticks
plt.tick_params(axis='x', which='both', pad=10, labelsize=10)


# Show the plot
# plt.tight_layout()
plt.show()