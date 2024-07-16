import pandas as pd
from matplotlib import pyplot as plt, ticker
from plots import get_axes_scaling_factor, get_divider

figure_width = 14
figure_height = 8
bar_width = 0.7


# data extraction
def get_duration(metrics):
    return metrics["durations"]["AlgorithmTotal"][0]


def get_remaining_vars_ratio(metrics):
    total = metrics["counters"]["InitVars"] - metrics["counters"]["MinVar"] + 1
    remaining = metrics["counters"]["FinalVars"] - metrics["counters"]["MinVar"] + 1
    return remaining / total


def get_relative_growth(metrics):
    initial = metrics["series"]["ClauseCounts"][0]
    final = metrics["series"]["ClauseCounts"][-1]
    return final / initial


# plots
def _plot_bar_group(ax: plt.Axes, data: list[tuple[list, str]]):
    bars = len(data)
    w = bar_width / bars
    first_pos = -bar_width / 2 + w / 2
    positions = list(range(len(data[0][0])))
    for i, collection in enumerate(data):
        points, label = collection
        pos = [x + first_pos + w * i for x in positions]
        ax.bar(pos, points, w, label=label)


def plot_durations(labels: list, data: dict[list]) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    _plot_bar_group(ax, [(points, setup) for setup, points in data.items()])
    factor, unit = get_axes_scaling_factor(ax)

    ax.set_title("Duration")
    ax.set_xlabel("input formulas")
    ax.set_ylabel(f"duration ({unit})")
    ax.xaxis.set_ticks(range(len(labels)), labels, rotation=90)
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(factor)))
    ax.set_xlim(left=-bar_width, right=len(labels) + bar_width)
    ax.set_ylim(bottom=0)

    fig.set_size_inches(figure_width, figure_height)
    fig.legend(loc="lower left")
    fig.tight_layout()
    return fig, axes


def plot_vars(labels: list, data: dict[list]) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    _plot_bar_group(ax, [(points, setup) for setup, points in data.items()])

    ax.set_title("Remaining variables to eliminate")
    ax.set_xlabel("input formulas")
    ax.set_ylabel(f"% remaining variables")
    ax.xaxis.set_ticks(range(len(labels)), labels, rotation=90)
    ax.set_xlim(left=-bar_width, right=len(labels) + bar_width)
    ax.set_ylim(bottom=0, top=1)

    fig.set_size_inches(figure_width, figure_height)
    fig.legend(loc="lower left")
    fig.tight_layout()
    return fig, axes


def plot_growth(labels: list, data: dict[list]) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    _plot_bar_group(ax, [(points, setup) for setup, points in data.items()])

    ax.set_title("Formula growth")
    ax.set_xlabel("input formulas")
    ax.set_ylabel("relative growth of # clauses")
    ax.xaxis.set_ticks(range(len(labels)), labels, rotation=90)
    ax.set_xlim(left=-bar_width, right=len(labels) + bar_width)
    ax.set_yscale("log")
    ax.set_ylim(bottom=0.4)

    fig.set_size_inches(figure_width, figure_height)
    fig.legend(loc="lower left")
    fig.tight_layout()
    return fig, axes


# interface
def extract_setup_summary_data(metrics: dict, data: dict[str, dict[str, tuple[float, float, float]]],
                               setup_key: str, input_key: str):
    if input_key not in data:
        data[input_key] = dict()
    data[input_key][setup_key] = (
        get_duration(metrics),
        get_remaining_vars_ratio(metrics),
        get_relative_growth(metrics),
    )


def create_setup_summary_table(data: dict[str, dict[str, tuple[float, float, float]]], setups: list[str]) -> pd.DataFrame:
    inputs = sorted(data.keys())
    complete_data = {(s, v): [] for s in setups for v in ["duration", "vars", "growth"]}
    for _, i_data in sorted(data.items()):
        for s in setups:
            duration, vars, growth = i_data.get(s, (float("nan"), float("nan"), float("nan")))
            complete_data[(s, "duration")].append(duration)
            complete_data[(s, "vars")].append(vars)
            complete_data[(s, "growth")].append(growth)
    df = pd.DataFrame(complete_data, index=inputs)
    return df


def get_data_for_metric(df: pd.DataFrame, setups: list[str], metric: str) -> dict[list]:
    data = {setup: df.loc[:,(setup, metric)].values for setup in setups}
    return data


def create_setup_summary_plots(df: pd.DataFrame, setups: list[str]) -> list[tuple[str, plt.Figure]]:
    labels = [i.rsplit('/', 1)[1] for i in df.index]
    figures = []

    fig_duration, _ = plot_durations(labels, get_data_for_metric(df, setups, "duration"))
    figures.append(("duration", fig_duration))

    fig_vars, _ = plot_vars(labels, get_data_for_metric(df, setups, "vars"))
    figures.append(("variables", fig_vars))

    fig_growth, _ = plot_growth(labels, get_data_for_metric(df, setups, "growth"))
    figures.append(("growth", fig_growth))

    return figures
