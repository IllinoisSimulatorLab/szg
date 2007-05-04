ALL = \
  AppFeatures.html \
  Architecture.html \
  arSZGClient.html \
  Bugs.html \
  ChangeLog.html \
  ClusterTest.html \
  CommLayer.html \
  CommunicationDetails.html \
  CompileCPP.html \
  DistributedOS.html \
  ExampleCode.html \
  GettingSoftware.html \
  Goals.html \
  GraphicsConfigExamples.html \
  GraphicsConfiguration.html \
  InputClasses.html \
  InputDeviceConfiguration.html \
  InputDevices-Drivers.html \
  InputDevices.html \
  InputFilters.html \
  Interaction.html \
  Navigation.html \
  Objects.html \
  PathConfiguration.html \
  PForth.html \
  PortabilityLayer.html \
  Programming.html \
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
	TrackerCoordConversion.html \
  VirtCompConfiguration.html \
  VisualStudioBuildVars.html \
  Standalone.html \
  SupportingLibraries.html \
  TrackingSim.html \
  UsingDocs.html \
  index.html

all: $(ALL)

ifeq ($(strip $(PYTHONEXE)),)
  PYTHONEXE=python
endif

%.html : txt2tags/%.t2t t2t.rc
	$(PYTHONEXE) txt2tags.py -t html -C t2t.rc -o $@ $<
