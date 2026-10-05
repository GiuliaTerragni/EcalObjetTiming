import ROOT
from DataFormats.FWLite import Events, Handle

ROOT.gROOT.SetBatch(True)

inputFile = "step4_miniaod_newTime.root"
outputFile = "ecalTiming_histograms.root"

# one method <-> one producer instance label
methods = {
    "Seed": "modifiedTimeSeed",
    "NoWeight": "modifiedTimeNotWeighted",
    "EnergyWeighted": "modifiedTimeEnergyWeighted",
    "SqrEnergyWeighted": "modifiedTimeSqrEnergyWeighted",
    "SqrtEnergyWeighted": "modifiedTimeSqrtEnergyWeighted",
    "ResEnergyWeighted": "modifiedTimeResEnergyWeighted",
}

methodColors = {
    "Seed": ROOT.kRed,
    "NoWeight": ROOT.kBlue,
    "EnergyWeighted": ROOT.kGreen + 2,
    "SqrEnergyWeighted": ROOT.kViolet + 2,
    "SqrtEnergyWeighted": ROOT.kOrange + 2,
    "ResEnergyWeighted": ROOT.kBlack,
}

objects = {
    "electron": ("slimmedElectrons", "std::vector<pat::Electron>"),
    "photon": ("slimmedPhotons", "std::vector<pat::Photon>"),
}

variables = {
    "ecalTime": lambda p: p.ecalTime(),
    "ecalTimeError": lambda p: p.ecalTimeError(),
    "ecalTimeNoOOTCorr": lambda p: p.ecalTimeNoOOTCorr(),
    "ecalTimeNoOOTCorrError": lambda p: p.ecalTimeNoOOTCorrError()
}

handles = {
    obj: Handle(cppType)
    for obj, (module, cppType) in objects.items()
}


# ----------------------------------------------------------------------
# Histograms
# ----------------------------------------------------------------------

hists = {}

for obj in objects:
    for var in variables:
        for method in methods:
            name = f"h_{obj}_{var}_{method}"
            title = (
                f"{obj.capitalize()} {var} ({method});"
                f"{var} [ns];"
                f"{obj.capitalize()}s"
            )

            hists[(obj, var, method)] = ROOT.TH1F(
                name,
                title,
                100,
                -5,
                5
            )


# ----------------------------------------------------------------------
# Event loop
# ----------------------------------------------------------------------

events = Events(inputFile)

nTotal = 0
nSelected = 0

for event in events:

    nTotal += 1

    # --------------------------------------------------------------
    # Electron selection
    #
    # Require:
    #   leading electron ET  > 20 GeV
    #   subleading electron ET > 10 GeV
    #
    # Sort explicitly because the input collection is not guaranteed
    # to be ordered in ET.
    # --------------------------------------------------------------

    event.getByLabel(
        "slimmedElectrons",
        handles["electron"]
    )

    electrons = list(handles["electron"].product())

    if len(electrons) < 2:
        continue

    electrons.sort(
        key=lambda electron: electron.et(),
        reverse=True
    )

    leadingElectron = electrons[0]
    subleadingElectron = electrons[1]

    if leadingElectron.et() <= 0.0:
        continue

    if subleadingElectron.et() <= 0.0:
        continue

    # Event passes the electron selection
    nSelected += 1


    # --------------------------------------------------------------
    # Fill histograms
    # --------------------------------------------------------------

    for obj, (module, _) in objects.items():

        for method, instance in methods.items():

            event.getByLabel(
                (module, instance, "NEWTIME"),
                handles[obj]
            )

            particles = list(handles[obj].product())

            for var, getValue in variables.items():

                for particle in particles:

                    hists[(obj, var, method)].Fill(
                        getValue(particle)
                    )


# ----------------------------------------------------------------------
# Gaussian fit
# ----------------------------------------------------------------------

def fitSigma(h):
    """
    Gaussian-fit the histogram (restricted to mean +/- 2*RMS)
    and return (mean, sigma).

    Falls back to (mean, RMS) if the distribution is too narrow
    or degenerate to fit.
    """

    mean = h.GetMean()
    rms = h.GetRMS()

    if h.GetEntries() < 2 or rms < 1e-6:
        return mean, rms

    fitResult = h.Fit(
        "gaus",
        "QNS",
        "",
        mean - 2 * rms,
        mean + 2 * rms
    )

    if not fitResult.Get() or fitResult.Status() != 0:
        return mean, rms

    return (
        fitResult.Parameter(1),
        fitResult.Parameter(2)
    )


# ----------------------------------------------------------------------
# Print results
# ----------------------------------------------------------------------

resolutions = {}

for obj in objects:
    for var in variables:
        for method in methods:

            h = hists[(obj, var, method)]

            resolutions[(obj, var, method)] = fitSigma(h)


print()
print(f"Total events:    {nTotal}")
print(f"Selected events: {nSelected}")

if nTotal > 0:
    print(
        f"Selection efficiency: "
        f"{100.0 * nSelected / nTotal:.2f}%"
    )

print()

print(
    f"{'object':<10}"
    f"{'variable':<20}"
    f"{'method':<20}"
    f"{'entries':>10}"
    f"{'mean [ns]':>12}"
    f"{'sigma [ns]':>12}"
)

for obj in objects:
    for var in variables:
        for method in methods:

            h = hists[(obj, var, method)]

            fitMean, sigma = resolutions[
                (obj, var, method)
            ]

            print(
                f"{obj:<10}"
                f"{var:<20}"
                f"{method:<20}"
                f"{int(h.GetEntries()):>10}"
                f"{fitMean:>12.4f}"
                f"{sigma:>12.4f}"
            )


# ----------------------------------------------------------------------
# Output file
# ----------------------------------------------------------------------

out = ROOT.TFile(
    outputFile,
    "RECREATE"
)

for h in hists.values():
    h.Write()


# ----------------------------------------------------------------------
# Overlay canvases
# ----------------------------------------------------------------------

canvases = []

for obj in objects:

    for var in variables:

        canvas = ROOT.TCanvas(
            f"c_{obj}_{var}_methods",
            f"{obj}: {var} method comparison",
            800,
            600
        )

        legend = ROOT.TLegend(
            0.6,
            0.65,
            0.88,
            0.88
        )

        maxY = max(
            hists[(obj, var, m)].GetMaximum()
            for m in methods
        ) * 1.1

        for i, method in enumerate(methods):

            h = hists[(obj, var, method)]

            _, sigma = resolutions[
                (obj, var, method)
            ]

            h.SetLineColor(
                methodColors[method]
            )

            h.SetLineWidth(2)
            h.SetMaximum(maxY)

            h.Draw(
                "HIST"
                if i == 0
                else "HIST SAME"
            )

            legend.AddEntry(
                h,
                f"{method}  (#sigma = {sigma:.3f} ns)",
                "l"
            )

        legend.Draw()

        canvas.Write()

        canvases.append(
            (canvas, legend)
        )


out.Close()

print()
print(f"Wrote {outputFile}")

