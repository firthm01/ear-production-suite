#pragma once

#include "JuceHeader.h"

#include "../../shared/binary_data.hpp"
#include "../../shared/components/ear_button.hpp"
#include "../../shared/components/look_and_feel/colours.hpp"
#include "../../shared/components/look_and_feel/fonts.hpp"
#include "item_store.hpp"
#include "programme_store.pb.h"

#include <memory>

namespace ear {
namespace plugin {
namespace ui {

class ElementOverview : public Component, private Timer {
 public:
  ElementOverview()
      : rotateLeftButton_(std::make_unique<EarButton>()),
        rotateRightButton_(std::make_unique<EarButton>()),
        currentViewingAngle_(180.f),
        endViewingAngle_(180.f),
        dirty_(true),
        overviewUpdate_(true) {
    rotateLeftButton_->setOffStateIcon(std::unique_ptr<Drawable>(
        Drawable::createFromImageData(binary_data::previous_icon_svg,
                                      binary_data::previous_icon_svgSize)));
    rotateLeftButton_->setOnStateIcon(std::unique_ptr<Drawable>(
        Drawable::createFromImageData(binary_data::previous_icon_svg,
                                      binary_data::previous_icon_svgSize)));
    rotateLeftButton_->onClick = [this]() { this->rotate(90.f); };

    rotateRightButton_->setOffStateIcon(
        std::unique_ptr<Drawable>(Drawable::createFromImageData(
            binary_data::next_icon_svg, binary_data::next_icon_svgSize)));
    rotateRightButton_->setOnStateIcon(
        std::unique_ptr<Drawable>(Drawable::createFromImageData(
            binary_data::next_icon_svg, binary_data::next_icon_svgSize)));
    rotateRightButton_->onClick = [this]() { this->rotate(-90.f); };

    addAndMakeVisible(rotateLeftButton_.get());
    addAndMakeVisible(rotateRightButton_.get());
  }

  int getSectorIndex(float value) {
    float limitedRange = static_cast<int>(std::roundf(value)) % 360;
    if ((limitedRange >= -315 && limitedRange < -225)) {
      return 1;
    } else if ((limitedRange >= -225 && limitedRange < -135)) {
      return 2;
    } else if ((limitedRange >= -135 && limitedRange < -45)) {
      return 3;
    } else if ((limitedRange >= -45 && limitedRange < 45)) {
      return 0;
    } else if (limitedRange >= 45 && limitedRange < 135) {
      return 1;
    } else if (limitedRange >= 135 && limitedRange < 225) {
      return 2;
    } else if (limitedRange >= 225 && limitedRange < 315) {
      return 3;
    } else {
      return 0;
    }
  }

  void paint(Graphics& g) override {
    std::unique_lock<std::mutex> lock(dirtyMutex_);
    if (overviewUpdate_ == false) {
      g.drawImageAt(cachedOverview_, 0, 0);
      return;
    }

    Image img(Image::PixelFormat::ARGB, getWidth(), getHeight(), false);
    Graphics glocal(img);

    {  // draw background
      glocal.drawImageAt(cachedBackground_, 0, 0);
    }
    if (overviewUpdate_ == true) {  // draw elements
      for (auto element : programme_.element()) {
        if (element.has_object()) {
          communication::ConnectionId id(element.object().connection_id());
          if (itemStore_.find(id) != itemStore_.end()) {
            auto item = itemStore_.at(id);
            auto colour = Colour(item.colour());
            if (item.has_ds_metadata()) {
              for (auto speaker : item.ds_metadata().speakers()) {
                if (!speaker.is_lfe()) {
                  auto position = speaker.position();
                  drawCircle(glocal, position, colour);
                }
              }
            } else if (item.has_obj_metadata()) {
              auto position = item.obj_metadata().position();
              drawCircle(glocal, position, colour);
            }
          }
        }
      }
    }
    {  // draw
      auto sphere = getLocalBounds()
                        .toFloat()
                        .removeFromBottom(126.f)
                        .removeFromLeft(156.f)
                        .withTrimmedLeft(50.f)
                        .withTrimmedBottom(20.f);
      glocal.setColour(EarColours::Sphere);
      drawSphereInRect(glocal, sphere, 1.f);
      glocal.setFont(EarFonts::Description);
      glocal.setColour(EarColours::Label);
      auto depthSphere = sphere.withSizeKeepingCentre(
          sphere.getWidth(), depthFactor_ * sphere.getHeight());

      std::vector<String> labels = {
          String::fromUTF8("Front"), String::fromUTF8("+90°"),
          String::fromUTF8("Back"), String::fromUTF8("–90°")};

      int startIndex = getSectorIndex(endViewingAngle_);
      auto frontLabelArea = depthSphere.withY(depthSphere.getBottom() + 2.f);
      glocal.drawText(labels.at(startIndex), frontLabelArea,
                      Justification::centredTop);
      auto rightLabelArea = sphere.withX(sphere.getRight() + 5.f);
      glocal.drawText(labels.at((startIndex + 1) % 4), rightLabelArea,
                      Justification::left);
      auto backLabelArea = depthSphere.withBottom(depthSphere.getY() - 2.f);
      glocal.drawText(labels.at((startIndex + 2) % 4), backLabelArea,
                      Justification::centredBottom);
      auto leftLabelArea = sphere.withRightX(sphere.getX() - 5.f);
      glocal.drawText(labels.at((startIndex + 3) % 4), leftLabelArea,
                      Justification::right);
    }
    dirty_ = false;
    g.drawImageAt(cachedOverview_, 0, 0);
    cachedOverview_ = img;
  }

  void drawSphereInRect(Graphics& g, Rectangle<float> rect, float linewidth) {
    g.drawEllipse(rect, linewidth);
    g.drawEllipse(rect.withSizeKeepingCentre(rect.getWidth(),
                                             depthFactor_ * rect.getHeight()),
                  linewidth);
  }

  void drawCircle(Graphics& g, proto::PolarPosition position, Colour colour) {
    position.set_azimuth(position.azimuth() + currentViewingAngle_);

    float x = std::sin(degreesToRadians(position.azimuth())) *
              std::cos(degreesToRadians(position.elevation())) *
              position.distance();
    float y = std::cos(-degreesToRadians(position.azimuth())) *
              std::cos(degreesToRadians(position.elevation())) *
              position.distance();
    float z =
        std::sin(degreesToRadians(position.elevation())) * position.distance();

    float circleRadiusScaled = circleRadius_ * ((y + 4.f) / 5.f);
    float xProjected = x * sphereArea_.getWidth() / 2.f;
    float yProjected = y * sphereArea_.getHeight() * depthFactor_ / 2.f -
                       z * sphereArea_.getHeight() / 2.f;

    juce::Rectangle circleArea(0.f, 0.f, circleRadiusScaled * 2.f,
                               circleRadiusScaled * 2.f);
    circleArea.translate(-circleRadiusScaled, -circleRadiusScaled);

    circleArea.translate(sphereArea_.getCentreX(), sphereArea_.getCentreY());
    circleArea.translate(xProjected, yProjected);
    g.setColour(
        colour.withAlpha(Emphasis::medium).darker(1.f - (y + 1.f) / 2.f));
    g.fillEllipse(circleArea);
  }

  void setProgramme(proto::Programme programme, ItemStore itemStore) {
    if (overviewUpdate_ == false) {
      return;
    }
    std::unique_lock<std::mutex> lock(dirtyMutex_, std::defer_lock);
    if (lock.try_lock()) {
      if (dirty_ == false) {
        programme_ = programme;
        itemStore_ = itemStore;
        repaint();
        dirty_ = true;
      }
    }
  }

  void rotate(float angle) {
    if (isTimerRunning()) {
      stopTimer();
    }
    startViewingAngle_ = currentViewingAngle_;
    endViewingAngle_ += angle;
    float animationRange = endViewingAngle_ - startViewingAngle_;
    if (std::abs(animationRange) >= 1.f) {
      startTimerHz(50);
    } else {
      currentViewingAngle_ = endViewingAngle_;
    }
  }

  void timerCallback() override {
    std::unique_lock<std::mutex> lock(dirtyMutex_, std::defer_lock);
    if (lock.try_lock()) {
      updateViewingAngle();
      if (dirty_ == false) {
        repaint();
        dirty_ = true;
      }
    }
  }

  void updateViewingAngle() {
    float animationRange = endViewingAngle_ - startViewingAngle_;
    currentViewingAngle_ += animationRange / 10.f;
    if (animationRange >= 0.f && currentViewingAngle_ > endViewingAngle_) {
      currentViewingAngle_ = endViewingAngle_;
      stopTimer();
    } else if (animationRange < 0.f &&
               currentViewingAngle_ < endViewingAngle_) {
      currentViewingAngle_ = endViewingAngle_;
      stopTimer();
    }
  }

  void resized() override {
    {
      Image img(Image::PixelFormat::ARGB, getWidth(), getHeight(), false);
      cachedOverview_ = img;
      Graphics g(img);

      // draw background
      auto area = getLocalBounds().toFloat();
      g.setColour(EarColours::Background);
      g.fillRect(area);
      float factorTop = 0.9;
      float factorBottom = 0.8;
      g.setColour(EarColours::Sphere);

      auto xPositionTop = getWidth() / 2.f;
      auto xPositionBottom = getWidth() / 2.f;
      float distanceTop = getWidth() / 30.f;
      float distanceBottom = getWidth() / 1.5f;
      for (int i = 0; i < 10; ++i) {
        g.drawLine(xPositionTop, 0.f, xPositionBottom, (float)getHeight());
        distanceTop *= factorTop;
        xPositionTop += distanceTop;
        distanceBottom *= factorBottom;
        xPositionBottom += distanceBottom;
      }
      xPositionTop = getWidth() / 2.f;
      xPositionBottom = getWidth() / 2.f;
      distanceTop = getWidth() / 20.f;
      distanceBottom = getWidth() / 1.5f;
      for (int i = 1; i < 10; ++i) {
        g.drawLine(xPositionTop, 0.f, xPositionBottom, (float)getHeight());
        distanceTop *= factorTop;
        xPositionTop -= distanceTop;
        distanceBottom *= factorBottom;
        xPositionBottom -= distanceBottom;
      }

      float yPosition = (3.f * getHeight()) / 4.f;
      float distance = getHeight() / 4.f;
      float factor = 0.68f;
      for (int i = 0; i < 15; ++i) {
        g.drawLine(0, yPosition, getWidth(), yPosition);
        distance *= factor;
        yPosition -= distance;
      }
      g.setGradientFill(ColourGradient::vertical(
          EarColours::Background, EarColours::Background.withAlpha(0.f),
          area.withTrimmedTop(getHeight() / 4)
              .withTrimmedBottom(getHeight() / 4)));
      g.fillRect(area);
      g.setColour(EarColours::ComboBoxPopupBackground);
      g.drawRect(area, 2);

      // draw sphere
      area.removeFromBottom(getHeight() / 8.f);

      float sphereSize;
      if (area.getWidth() > area.getHeight()) {
        sphereSize = area.getHeight() - 100;
      } else {
        sphereSize = area.getWidth() - 100;
      }
      sphereArea_ = area.withSizeKeepingCentre(sphereSize, sphereSize);
      g.setColour(EarColours::Sphere);
      drawSphereInRect(g, sphereArea_, 4.f);

      // save Image
      cachedBackground_ = img;
    }
    auto area = getLocalBounds().removeFromBottom(50).withTrimmedBottom(20);
    rotateLeftButton_->setBounds(area.removeFromLeft(50).withTrimmedLeft(20));
    area.removeFromLeft(106);  // skip sphere
    rotateRightButton_->setBounds(area.removeFromLeft(30));
  }

  void mouseDown(const MouseEvent& event) override {
    if (rotateLeftButton_->isMouseButtonDown() ||
        rotateRightButton_->isMouseButtonDown()) {
      return;
    }

    overviewUpdate_ = !overviewUpdate_;

    if (overviewUpdate_ == false) {
      rotateLeftButton_->onClick = nullptr;
      rotateRightButton_->onClick = nullptr;
    } else {
      rotateLeftButton_->onClick = [this]() { this->rotate(90.f); };
      rotateRightButton_->onClick = [this]() { this->rotate(-90.f); };
    }
  }

 private:
  proto::Programme programme_;
  ItemStore itemStore_;

  std::unique_ptr<EarButton> rotateLeftButton_;
  std::unique_ptr<EarButton> rotateRightButton_;

  Image cachedBackground_;
  Image cachedOverview_;

  juce::Rectangle<float> sphereArea_;
  const float depthFactor_ = 0.3f;
  const float circleRadius_ = 15.f;

  float startViewingAngle_;
  float endViewingAngle_;
  float currentViewingAngle_;

  std::mutex dirtyMutex_;

  bool dirty_;
  bool overviewUpdate_;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ElementOverview)
};

}  // namespace ui
}  // namespace plugin
}  // namespace ear
