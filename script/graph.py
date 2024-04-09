import pandas as pd
import matplotlib.pyplot as plt

def createLatencyGraph(df) -> None:
    plt.plot(df['num_threads'], df['avg_latency'], label='Average Latency', marker='o')

    # Add labels and title
    plt.xlabel('Number of Threads')
    plt.ylabel('Latency (ms)')

    # Add legend
    plt.legend()

    # Save the latency graph
    plt.savefig(f'../img/latency_graph.png', bbox_inches='tight', pad_inches=0.5)
    plt.clf()

if __name__ == "__main__":
    # Read data into panda dataframe
    column_name = ["num_threads", "avg_latency"]
    file = "../data_preprocessed.csv"
    df = pd.read_csv(file, header=None, names=column_name)
    df_grouped = df.groupby("num_threads").mean().reset_index()
    df_grouped["avg_latency"] = df_grouped["avg_latency"] / 1000
    createLatencyGraph(df_grouped)
