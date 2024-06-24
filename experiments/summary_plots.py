from matplotlib import pyplot as plt, ticker
from plots import get_axes_scaling_factor, get_divider


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
def plot_durations(labels: list, data: dict[list]) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    for setup, points in data.items():
        ax.plot(points, label=setup)
    factor, unit = get_axes_scaling_factor(ax)

    ax.set_title("Duration")
    ax.set_xlabel("input formulas")
    ax.set_ylabel(f"duration ({unit})")
    ax.xaxis.set_ticklabels(labels)
    ax.yaxis.set_major_formatter(ticker.FuncFormatter(get_divider(factor)))
    ax.set_ylim(bottom=0)

    fig.legend(loc="lower left")
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.1 * (len(data) // 2))
    return fig, axes


def plot_vars(labels: list, data: dict[list]) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    for setup, points in data.items():
        ax.plot(points, label=setup)

    ax.set_title("Remaining variables to eliminate")
    ax.set_xlabel("input formulas")
    ax.set_ylabel(f"% remaining variables")
    ax.xaxis.set_ticklabels(labels)
    ax.set_ylim(bottom=0, top=1)

    fig.legend(loc="lower left")
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.1 * (len(data) // 2))
    return fig, axes


def plot_growth(labels: list, data: dict[list]) -> tuple[plt.Figure, list[plt.Axes]]:
    axes = []

    sub: tuple[plt.Figure, plt.Axes] = plt.subplots()
    fig, ax = sub
    axes.append(ax)
    for setup, points in data.items():
        ax.plot(points, label=setup)

    ax.set_title("Formula growth")
    ax.set_xlabel("input formulas")
    ax.set_ylabel("relative growth of # clauses")
    ax.xaxis.set_ticklabels(labels)
    ax.set_ylim(bottom=0, top=1)

    fig.legend(loc="lower left")
    fig.tight_layout()
    fig.subplots_adjust(bottom=0.1 * (len(data) // 2))
    return fig, axes


# interface
def extract_setup_summary_data(metrics: dict, data: dict, setup_key, input_key):
    if input_key not in data:
        data[input_key] = dict()
    data[input_key][setup_key] = (
        get_duration(metrics),
        get_remaining_vars_ratio(metrics),
        get_relative_growth(metrics),
    )


def prepare_setup_summary_data(data: dict) -> tuple[list, dict[dict[list]]]:
    inputs = sorted(data.keys)
    setups = {s_key for i_data in data.values() for s_key in i_data.keys()}
    complete_data = {
        "duration": {s: [] for s in setups},
        "vars": {s: [] for s in setups},
        "growth": {s: [] for s in setups},
    }
    for i_data in sorted(data.values()):
        for s in setups:
            duration, vars, growth = i_data.get(s, default=(float("nan"), float("nan"), float("nan")))
            complete_data["duration"][s] = duration
            complete_data["vars"][s] = vars
            complete_data["growth"][s] = growth
    return inputs, complete_data


def create_setup_summary_plots(data: dict) -> list[tuple[str, plt.Figure]]:
    inputs, complete_data = prepare_setup_summary_data(data)
    figures = []

    fig_duration, _ = plot_durations(inputs, complete_data["duration"])
    figures.append(("duration", fig_duration))

    fig_vars, _ = plot_vars(inputs, complete_data["vars"])
    figures.append(("variables", fig_vars))

    fig_growth, _ = plot_growth(inputs, complete_data["growth"])
    figures.append(("growth", fig_growth))

    return figures
