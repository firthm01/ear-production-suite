#include "monitoring_plugin_editor.hpp"

#include "../../shared/components/look_and_feel/colours.hpp"
#include "../../shared/components/look_and_feel/fonts.hpp"
#include "../../shared/helper/properties_file.hpp"
#include "detail/constants.hpp"
#include "monitoring_plugin_processor.hpp"
#include "speaker_setups.hpp"

#include <string>

using namespace ear::plugin::ui;

EarMonitoringAudioProcessorEditor::EarMonitoringAudioProcessorEditor(
    EarMonitoringAudioProcessor* p)
    : AudioProcessorEditor(p),
      p_(p),
      header_(std::make_unique<EarHeader>()),
      onBoardingButton_(std::make_unique<EarButton>()),
      onBoardingOverlay_(std::make_unique<Overlay>()),
      onBoardingContent_(std::make_unique<Onboarding>()),
      speakerMeterBoxTop_(
          std::make_unique<SpeakerMeterBox>(String::fromUTF8("1–12"))),
      speakerMeterBoxBottom_(
          std::make_unique<SpeakerMeterBox>(String::fromUTF8("13–24"))),
      propertiesFileLock_(
          std::make_unique<InterProcessLock>("EPS_preferences")),
      propertiesFile_(getPropertiesFile(propertiesFileLock_.get())) {
  header_->setText(" Binaural Monitoring");

  onBoardingButton_->setButtonText("?");
  onBoardingButton_->setShape(EarButton::Shape::Circular);
  onBoardingButton_->setFont(
      font::RobotoSingleton::instance().getRegular(20.f));
  onBoardingButton_->onClick = [&]() { onBoardingOverlay_->setVisible(true); };

  onBoardingOverlay_->setContent(onBoardingContent_.get());
  onBoardingOverlay_->setWindowSize(706, 596);
  onBoardingOverlay_->setHeaderText(
      String::fromUTF8("Welcome – do you need help?"));
  onBoardingOverlay_->onClose = [&]() {
    onBoardingOverlay_->setVisible(false);
    propertiesFile_->setValue("showOnboarding", false);
  };
  if (!propertiesFile_->containsKey("showOnboarding")) {
    onBoardingOverlay_->setVisible(true);
  }
  onBoardingContent_->addListener(this);

  addAndMakeVisible(header_.get());
  addAndMakeVisible(onBoardingButton_.get());
  addChildComponent(onBoardingOverlay_.get());

  addAndMakeVisible(speakerMeterBoxTop_.get());
  //addAndMakeVisible(speakerMeterBoxBottom_.get());

  auto speakers = ear::plugin::speakerSetupByName(SPEAKER_LAYOUT).speakers;
  for (int i = 0; i < speakers.size(); ++i) {
    std::string ituLabel = speakers.at(i).label;
    auto pos = ituLabel.find("-");
    if (pos < ituLabel.size()) {
      ituLabel.replace(pos, 1, "–");
    }
    speakerMeters_.push_back(std::make_unique<SpeakerMeter>(
        String(i + 1), speakers.at(i).spLabel, ituLabel));
    speakerMeters_.back()->getLevelMeter()->setMeter(p->getLevelMeter(), i);
    addAndMakeVisible(speakerMeters_.back().get());
  }
  setSize(735, 650);
}

EarMonitoringAudioProcessorEditor::~EarMonitoringAudioProcessorEditor() {}

void EarMonitoringAudioProcessorEditor::paint(Graphics& g) {
  g.fillAll(EarColours::Background);
}

void EarMonitoringAudioProcessorEditor::resized() {
  auto area = getLocalBounds();
  onBoardingOverlay_->setBounds(area);
  area.reduce(5, 5);
  auto headingArea = area.removeFromTop(55);
  onBoardingButton_->setBounds(
      headingArea.removeFromRight(39).removeFromBottom(39));
  header_->setBounds(headingArea);

  area.removeFromTop(10);

  auto topArea = area.removeFromTop(290).reduced(5, 5);
  speakerMeterBoxTop_->setBounds(topArea);
  auto bottomArea = area.removeFromTop(290).reduced(5, 5);
  speakerMeterBoxBottom_->setBounds(bottomArea);
  topArea.removeFromTop(66);
  topArea.removeFromLeft(30);
  topArea.removeFromBottom(15);
  for (int i = 0; i < 12 && i < speakerMeters_.size(); ++i) {
    speakerMeters_.at(i)->setBounds(topArea.removeFromLeft(50));
    topArea.removeFromLeft(5);
  }
  bottomArea.removeFromTop(66);
  bottomArea.removeFromLeft(30);
  bottomArea.removeFromBottom(15);
  for (int i = 12; i < 24 && i < speakerMeters_.size(); ++i) {
    speakerMeters_.at(i)->setBounds(bottomArea.removeFromLeft(50));
    bottomArea.removeFromLeft(5);
  }
}

void EarMonitoringAudioProcessorEditor::endButtonClicked(
    Onboarding* onboarding) {
  onBoardingOverlay_->setVisible(false);
  propertiesFile_->setValue("showOnboarding", false);
}
void EarMonitoringAudioProcessorEditor::moreButtonClicked(
    Onboarding* onboarding) {
  onBoardingOverlay_->setVisible(false);
  URL(ear::plugin::detail::MORE_INFO_URL).launchInDefaultBrowser();
}
