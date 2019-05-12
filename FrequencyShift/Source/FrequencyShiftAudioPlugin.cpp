#include "FrequencyShiftAudioPlugin.h"



//class FrequencyShifterParameterContainerFactory : public ParameterContainerComponentFactory  {
//	ParameterContainerComponent* create(PluginParameters* valueTreeState) override {
//		return new FrequencySlider(valueTreeState, Colour::fromString(TEXT_COLOUR), 30); 				
//	}
//};
	
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{	
	//create shared pointer here and pass to processor and factory...which should actually just be the container
	SpectralAudioPlugin::DependencyCreator dependencyCreator = [](SpectralAudioPlugin* plugin) {
		auto pluginParams = std::make_shared<FrequencyShiftPluginParameters>(plugin);		

		return std::make_unique<SpectralAudioPlugin::Dependencies>(
			pluginParams,
			std::make_unique<FrequencyShiftProcessor>(SpectralAudioPlugin::FFT_OVERLAPS, SpectralAudioPlugin::N_CHANS),
			std::make_unique<FrequencySlider>(pluginParams, Colour::fromString(TEXT_COLOUR), 30)
		);

		//std::make_unique<FrequencyShiftProcessor>(SpectralAudioPlugin::FFT_OVERLAPS, SpectralAudioPlugin::N_CHANS),
		//std::make_unique<FrequencyShifterParameterContainerFactory>(),
	};
	
	return new SpectralAudioPlugin(
		dependencyCreator,
		//std::make_unique<FrequencyShiftProcessor>(SpectralAudioPlugin::FFT_OVERLAPS, SpectralAudioPlugin::N_CHANS),
		//std::make_unique<FrequencyShifterParameterContainerFactory>(),
		Array<int>(128, 256, 512) // fft sizes to remove
	);
}
