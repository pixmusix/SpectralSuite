#include "SpectralAudioPlugin.h"

// avoiding circular declaration
#include "SpectralAudioPluginUi.h"

const int SpectralAudioPlugin::FFT_OVERLAPS = 4;
const int SpectralAudioPlugin::INIT_FFT_INDEX = 4; // 2048 

//==============================================================================
SpectralAudioPlugin::SpectralAudioPlugin(
	//std::unique_ptr<SpectralAudioProcessor> audioProcessor, std::unique_ptr<ParameterContainerComponentFactory> parameterComponentFactory
	DependencyFactory* dependencies
//	Array<int> fftSizesToRemove
)
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", AudioChannelSet::stereo(), true)
#endif
	),

#endif
	m_fftChoiceAdapter(INIT_FFT_INDEX),
	//parameters(*this, nullptr),	
	m_fftSwitcher(this),
    m_internalBufferReadWriteIndex(0),
	m_versionCheckThread(VersionCode, "https://www.andrewreeman.com/spectral_suite_publish.json"),
    m_dependencyFactory(dependencies),
    m_shouldUpdateOverlapCount(false)
{			
	

	//FileLogger* logger = new FileLogger(FileLogger::getSystemLogFileFolder().getChildFile("logs")
	//	.getChildFile("spectral_suite" + Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S"))
	//	.withFileExtension(".log")
	//	.getNonexistentSibling(),
	//	"Log started", 0);

	File logFile = FileLogger::getSystemLogFileFolder().getChildFile("SpectralSuite").getChildFile("spectral_suite.log");
	m_logger = std::unique_ptr<FileLogger>(
		new FileLogger(
		logFile,
			"Log started"
		)
	);
	Logger::setCurrentLogger(m_logger.get());
    this->initialiseDependencies();
}

SpectralAudioPlugin::~SpectralAudioPlugin()
{    
	Logger::setCurrentLogger(nullptr);
}

/* FFT Switcher methods */
void SpectralAudioPlugin::switchFftSize()
{
	setFftSize(m_fftChoiceAdapter.fftSize());
    
    if(m_parameterUiComponent != nullptr) {
        m_parameterUiComponent->onFftSizeChanged();
    }
}

void SpectralAudioPlugin::switchOverlapCount() {
    m_audioProcessorInteractor->switchOverlapCount();
    
    const int hopSize = m_audioProcessorInteractor->getHopSize();
	for(std::vector<float>& output : m_output)
	{
		output.resize(hopSize, 0.f);
	}
	for(std::vector<float>& input : m_input)
	{
		input.resize(hopSize, 0.f);
	}

	setLatencySamples(m_audioProcessorInteractor->getFftSize() + hopSize);
    m_shouldUpdateOverlapCount = false;
}

/* Public methods */

const String SpectralAudioPlugin::getName() const
{
    return JucePlugin_Name;
}

bool SpectralAudioPlugin::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool SpectralAudioPlugin::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool SpectralAudioPlugin::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double SpectralAudioPlugin::getTailLengthSeconds() const
{
    return 0.0;
}

int SpectralAudioPlugin::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int SpectralAudioPlugin::getCurrentProgram()
{
    return 0;
}

void SpectralAudioPlugin::setCurrentProgram (int)
{
}

const String SpectralAudioPlugin::getProgramName (int)
{
    return {};
}

void SpectralAudioPlugin::changeProgramName (int, const String&)
{
}

void SpectralAudioPlugin::prepareToPlay (double sampleRate, int)
{    
	m_output.clear();
	m_input.clear();
	for (
		int outputChannelCount = getBusesLayout().getMainOutputChannels();		
		outputChannelCount > 0;
		outputChannelCount--
	)
	{
		m_output.push_back(std::vector<float>());
		m_input.push_back(std::vector<float>());
	}

	const int fftSize = m_fftChoiceAdapter.fftSize();	
	m_audioProcessorInteractor->prepareToPlay(fftSize, (int)sampleRate, getBusesLayout().getMainOutputChannels());	
	setFftSize(fftSize);		
}

void SpectralAudioPlugin::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool SpectralAudioPlugin::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else  
    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void SpectralAudioPlugin::emptyOutputs() {
	for(auto& output : m_output)
	{
		std::fill(output.begin(), output.end(), 0.f);
	}	
}

void SpectralAudioPlugin::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiBuffer)
{
	if (m_fftSwitcher.isThreadRunning()) {        
		m_internalBufferReadWriteIndex = 0;
		return; 
	}

	if (m_fftChoiceAdapter.shouldChangeFft()) {
		m_fftSwitcher.switchFftSize();
		return;
	}
 
    if(m_shouldUpdateOverlapCount) {
        m_fftSwitcher.switchOverlapCount();
        return;
    }
    
    if(!midiBuffer.isEmpty()) {
        m_audioProcessorInteractor->receivedMidi(midiBuffer);
    }        

	const int hopSize = m_audioProcessorInteractor->getHopSize();
	const int numChannels = buffer.getNumChannels();
	float** audio = buffer.getArrayOfWritePointers();		
	int numSamples = buffer.getNumSamples();
	int ioVSTBuffers = 0;

	while (numSamples--) {		
		if (m_internalBufferReadWriteIndex >= hopSize) {
			m_internalBufferReadWriteIndex = 0;
			emptyOutputs();
			m_audioProcessorInteractor->process(&m_input, &m_output);
		}

		for (int channel = 0; channel < numChannels; channel++)
		{
			m_input[channel][m_internalBufferReadWriteIndex] = audio[channel][ioVSTBuffers];
			audio[channel][ioVSTBuffers] = m_output[channel][m_internalBufferReadWriteIndex];
		}
		
		ioVSTBuffers++;
		m_internalBufferReadWriteIndex++;	
	}			
}

bool SpectralAudioPlugin::hasEditor() const
{
    return true;
}

AudioProcessorEditor* SpectralAudioPlugin::createEditor()
{
    m_parameterUiComponent = m_dependencyFactory->createUi(this);
	return new SpectralAudioPluginUi(*this, parameters.get(), m_parameterUiComponent);
}

//==============================================================================
void SpectralAudioPlugin::getStateInformation (MemoryBlock& destData)
{    
	auto state = parameters->copyState();
	//AudioParameterFloat* shift = (AudioParameterFloat*)parameters->getParameter("shift");
	//AudioParameterFloat* min = (AudioParameterFloat*)parameters.getParameter("shiftMinRange");
	//AudioParameterFloat* max = (AudioParameterFloat*)parameters.getParameter("shiftMaxRange");
	std::unique_ptr<XmlElement> xml(state.createXml());
	
	// encode range

	//XmlElement* shiftXmlElement = xml->getChildByAttribute("id", "shift");	
	//shiftXmlElement->setAttribute("minRange", min->get());
	//shiftXmlElement->setAttribute("maxRange", max->get());

	copyXmlToBinary(*xml, destData);
}

void SpectralAudioPlugin::setStateInformation (const void* data, int sizeInBytes)
{ 
	std::unique_ptr<XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

	if ( xmlState.get() == nullptr ) { return; }
	if ( !xmlState->hasTagName(parameters->getState().getType()) ) { return;}
		
	parameters->replaceState(ValueTree::fromXml(*xmlState));	
	AudioProcessorEditor* editor =  this->getActiveEditor();
	if (editor != nullptr) {
		((SpectralAudioPluginUi*)editor)->onAudioValueTreeStateLoadedFromXmlState(parameters.get(), xmlState.get());		
	}
}

void SpectralAudioPlugin::setFftSize(int size) {	
	m_audioProcessorInteractor->setFftSize(size);
	const int hopSize = m_audioProcessorInteractor->getHopSize();

	for(std::vector<float>& output : m_output)
	{
		output.resize(hopSize, 0.f);
	}

	for(std::vector<float>& input : m_input)
	{
		input.resize(hopSize, 0.f);
	}

	setLatencySamples(size + hopSize);
}

void SpectralAudioPlugin::checkForUpdates(VersionCheckThread::Listener* listener) {
	m_versionCheckThread.setListener(listener);
	m_versionCheckThread.startThread();
}

void SpectralAudioPlugin::initialiseDependencies() {
    
    parameters = m_dependencyFactory->createParams(this);
    m_audioProcessorInteractor = m_dependencyFactory->createProcessor(this);
    
    auto fftSizesToRemove = m_dependencyFactory->fftSizesToNotInclude();
    m_fftChoiceAdapter.remove(fftSizesToRemove);

    //m_audioProcessor->createParameters(parameters.get());
    parameters->createAndAddParameter(
        std::make_unique<AudioParameterChoice>(
            "fft", "FFT Size", m_fftChoiceAdapter.fftStrings(), m_fftChoiceAdapter.currentIndex()
        )
    );
    
    auto fftChoices = (AudioParameterChoice*)parameters->getParameter("fft");
    m_fftChoiceAdapter.listen(fftChoices);
    
    auto valueTree = ValueTree(
        Identifier(
            this->getName().removeCharacters(" ")
        )
    );
    parameters->replaceState(valueTree);
    //m_ui = new SpectralAudioPluginUi(*this, parameters.get(), m_parameterUiComponent);
}
