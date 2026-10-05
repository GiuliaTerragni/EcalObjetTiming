import ROOT
from DataFormats.FWLite import Events, Handle

ROOT.gROOT.SetBatch(True)

inputFile = "step4_miniaod_newTime.root"
outputFile = "ecalTiming_histograms.root"

# one method <-> one producer instance label
methods = {
    "Fixed": "modifiedTimeFixed",
    "Seed": "modifiedTimeSeed",
    "EnergyWeighted": "modifiedTimeEnergyWeighted",
    "SqrtEnergyWeighted": "modifiedTimeSqrtEnergyWeighted",
}
methodColors = {
    "Fixed": ROOT.kBlack,
    "Seed": ROOT.kRed,
    "EnergyWeighted": ROOT.kBlue,
    "SqrtEnergyWeighted": ROOT.kGreen + 2,
}

objects = {
    "electron": ("slimmedElectrons", "std::vector<pat::Electron>"),
    "photon": ("slimmedPhotons", "std::vector<pat::Photon>"),
}

variables = {
    "ecalTime": lambda p: p.ecalTime(),
    "ecalTimeNoOOTCorr": lambda p: p.ecalTimeNoOOTCorr(),
}

handles = {obj: Handle(cppType) for obj, (module, cppType) in objects.items()}

hists = {}
for obj in objects:
    for var in variables:
        for method in methods:
            name = f"h_{obj}_{var}_{method}"
            title = f"{obj.capitalize()} {var} ({method});{var} [ns];{obj.capitalize()}s"
            hists[(obj, var, method)] = ROOT.TH1F(name, title, 100, -5, 5)

events = Events(inputFile)

for event in events:
    for obj, (module, _) in objects.items():
        for method, instance in methods.items():
            event.getByLabel((module, instance, "TEST"), handles[obj])
            particles = list(handles[obj].product())
            for var, getValue in variables.items():
                for particle in particles:
                    hists[(obj, var, method)].Fill(getValue(particle))

def fitSigma(h):
    """Gaussian-fit the histogram (restricted to mean +/- 2*RMS) and return (mean, sigma).
    Falls back to (mean, RMS) if the distribution is too narrow/degenerate to fit
    (e.g. the Fixed method, which is a single spike)."""
    mean = h.GetMean()
    rms = h.GetRMS()
    if h.GetEntries() < 2 or rms < 1e-6:
        return mean, rms
    fitResult = h.Fit("gaus", "QNS", "", mean - 2 * rms, mean + 2 * rms)
    if not fitResult.Get() or fitResult.Status() != 0:
        return mean, rms
    return fitResult.Parameter(1), fitResult.Parameter(2)

resolutions = {}
for obj in objects:
    for var in variables:
        for method in methods:
            h = hists[(obj, var, method)]
            resolutions[(obj, var, method)] = fitSigma(h)

print(f"\n{'object':<10}{'variable':<20}{'method':<20}{'entries':>10}{'mean [ns]':>12}{'sigma [ns]':>12}")
for obj in objects:
    for var in variables:
        for method in methods:
            h = hists[(obj, var, method)]
            fitMean, sigma = resolutions[(obj, var, method)]
            print(f"{obj:<10}{var:<20}{method:<20}{int(h.GetEntries()):>10}{fitMean:>12.4f}{sigma:>12.4f}")

out = ROOT.TFile(outputFile, "RECREATE")

for h in hists.values():
    h.Write()

# overlay canvas comparing methods, per object and per variable.
# Fixed is excluded here: its spike is much taller than the real
# distributions and squashes them on the shared y-axis.
plotMethods = [m for m in methods if m != "Fixed"]

canvases = []
for obj in objects:
    for var in variables:
        canvas = ROOT.TCanvas(f"c_{obj}_{var}_methods", f"{obj}: {var} method comparison", 800, 600)
        legend = ROOT.TLegend(0.6, 0.65, 0.88, 0.88)

        maxY = max(hists[(obj, var, m)].GetMaximum() for m in plotMethods) * 1.1

        for i, method in enumerate(plotMethods):
            h = hists[(obj, var, method)]
            _, sigma = resolutions[(obj, var, method)]
            h.SetLineColor(methodColors[method])
            h.SetLineWidth(2)
            h.SetMaximum(maxY)
            h.Draw("HIST" if i == 0 else "HIST SAME")
            legend.AddEntry(h, f"{method}  (#sigma = {sigma:.3f} ns)", "l")

        legend.Draw()
        canvas.Write()
        canvases.append((canvas, legend))

out.Close()

print(f"\nWrote {outputFile}")
