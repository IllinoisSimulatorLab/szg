ALL = \
  AppFeatures.html \
  Architecture.html \
  Bugs.html \
  ChangeLog.html \
  ClusterTest.html \
  CommLayer.html \
  CompileCPP.html \
  ExampleCode.html \
  Future.html \
  GettingSoftware.html \
  Goals.html \
  InputClasses.html \
  InputDevices-Drivers.html \
  InputDevices.html \
  InputFilters.html \
  Interaction.html \
  Navigation.html \
	Objects.html \
	PForth.html \
	Phleet.html \
	PhleetIntro.html \
	PortabilityLayer.html \
	Programming.html \
	Phleet.html \
	PySZG.html \
	QuickPort.html \
	RandomNumbers.html \
	SceneGraph.html \
	SceneGraphTutorial.html \
	SharedWorldTutorial.html \
	Sound.html \
	SystemConfiguration.html \
	SyzygyEnvironmentVariables.html \
	szgview.html \
	Testing.html \
	VisualStudioBuildVars.html \
  Standalone.html \
  SupportingLibraries.html \
  TrackingSim.html \
  UsingDocs.html \
  index.html

all: $(ALL)

%.html : txt2tags/%.t2t
	python txt2tags.py -t html --outfile $@ $<


