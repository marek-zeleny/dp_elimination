import pandas as pd
from matplotlib import pyplot as plt, ticker
from plots import get_axes_scaling_factor, get_divider

figure_width = 14
figure_height = 8
bar_width = 0.7


# data extraction
def get_duration(df: pd.DataFrame, setups: str) -> dict[list]:
    data = {
        s: df.loc[:, (s, "duration")].values
        for s in setups
    }
    return data


def get_remaining_vars_ratio(df: pd.DataFrame, setups: str) -> dict[list]:
    vars = df.loc[:, ("vars", "")]
    aux_vars = df.loc[:, ("aux_vars", "")]
    prot_vars = vars - aux_vars
    data = {
        s: ((df.loc[:, (s, "end_vars")] - prot_vars) / aux_vars).values
        for s in setups
    }
    return data


def get_relative_growth(df: pd.DataFrame, setups: str) -> dict[list]:
    size = df.loc[:, ("size", "")]
    data = {
        s: (df.loc[:, (s, "end_size")] / size).values
        for s in setups
    }
    return data


#plots
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
def create_setup_summary_plots(df: pd.DataFrame, setups: list[str]) -> list[tuple[str, plt.Figure]]:
    labels = [i.rsplit('/', 1)[1] for i in df.index]
    figures = []

    fig_duration, _ = plot_durations(labels, get_duration(df, setups))
    figures.append(("duration", fig_duration))

    fig_vars, _ = plot_vars(labels, get_remaining_vars_ratio(df, setups))
    figures.append(("variables", fig_vars))

    fig_growth, _ = plot_growth(labels, get_relative_growth(df, setups))
    figures.append(("growth", fig_growth))

    return figures
