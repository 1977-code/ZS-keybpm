#include "PluginEditor.h"

using namespace juce;

namespace
{
    // Русские подсказки — интерфейс английский, как принято в плагинах,
    // а объяснения на языке студии.
    const String tempoTip = String::fromUTF8 (
        "Темп, посчитанный по самому материалу, а не взятый у хоста. Строка HOST — темп проекта: "
        "рядом видно расхождение, поэтому лупу или сэмпл можно проверить, не считая на слух. "
        "Confidence — какая доля всего услышанного согласна с этой цифрой.");

    const String keyTip = String::fromUTF8 (
        "Тональность по накопленной хроме. ALT — второй кандидат: почти все ошибки определения "
        "тональности — это параллельная или одноимённая, и если он близко, доверять цифре стоит "
        "меньше. Camelot — код для сведения по кругу квинт (8A + 8B, 8A + 9A).");

    const String wheelTip = String::fromUTF8 (
        "Двенадцать нот по кругу квинт. Ноты одной тональности стоят рядом, поэтому настоящая "
        "тональность видна как одна яркая дуга, а ошибка — как разбросанные лучи. "
        "Золотом — ступени найденной тональности, длинный луч — тоника.");

    const String stripTip = String::fromUTF8 (
        "Четыре секунды функции атак — то, из чего считается темп, — и найденная сетка бита поверх. "
        "Если риски стоят на пиках, темп верный. Точка справа пульсирует в такт и замирает вместе "
        "с транспортом.");

    const String rangeTip = String::fromUTF8 (
        "В какой октаве показывать темп. 140 и 70 — одно и то же исполнение, и какой из них "
        "называть темпом, решает не плагин. Диапазон ровно 2:1, поэтому ответ всегда один.");

    const String notationTip = String::fromUTF8 (
        "Писать через диезы или бемоли: F# или Gb.");

    const String holdTip = String::fromUTF8 (
        "Заморозить показания. Анализ продолжается, просто цифры перестают меняться — удобно, "
        "чтобы записать результат.");

    const String resetTip = String::fromUTF8 (
        "Забыть всё услышанное и начать заново. Нужно при смене трека: накопленная хрома и "
        "гистограмма темпа живут десятками секунд.");
}

//==============================================================================
ZsKeyBpmAudioProcessorEditor::Content::Content (ZsKeyBpmAudioProcessor& p)
    : plugin (p),
      background (String ("v") + JucePlugin_VersionString),
      rangeStrip    (*p.apvts.getParameter (zs::params::range)),
      notationStrip (*p.apvts.getParameter (zs::params::notation)),
      holdAttachment (*p.apvts.getParameter (zs::params::hold), holdButton, nullptr)
{
    holdButton.setClickingTogglesState (true);
    resetButton.onClick = [this] { plugin.resetAnalysis(); };

    addAndMakeVisible (background);
    addAndMakeVisible (bpm);
    addAndMakeVisible (key);
    addAndMakeVisible (wheel);
    addAndMakeVisible (strip);
    addAndMakeVisible (rangeStrip);
    addAndMakeVisible (notationStrip);
    addAndMakeVisible (holdButton);
    addAndMakeVisible (resetButton);

    bpm.setTooltip (tempoTip);
    key.setTooltip (keyTip);
    wheel.setTooltip (wheelTip);
    strip.setTooltip (stripTip);
    rangeStrip.setTooltipForAll (rangeTip);
    notationStrip.setTooltipForAll (notationTip);
    holdButton.setTooltip (holdTip);
    resetButton.setTooltip (resetTip);

    startTimerHz (30);
}

void ZsKeyBpmAudioProcessorEditor::Content::resized()
{
    using namespace zs::layout;

    background.setBounds (getLocalBounds());

    bpm.setBounds (tempoCardBounds());
    wheel.setBounds (chromaWheelBounds());
    key.setBounds (keyReadoutBounds());
    strip.setBounds (stripBounds());

    rangeStrip.setBounds (rangeStripBounds());
    notationStrip.setBounds (notationStripBounds());
    holdButton.setBounds (holdButtonBounds());
    resetButton.setBounds (resetButtonBounds());
}

void ZsKeyBpmAudioProcessorEditor::Content::timerCallback()
{
    // One read, one truth: every readout below sees the same instant.
    const auto snapshot = plugin.getSnapshot();
    const auto notation = plugin.getNotation();

    bpm.update (snapshot, plugin.getHostBpm(), plugin.isHostPlaying());
    key.update (snapshot, notation);
    wheel.update (snapshot, notation);
    strip.update (snapshot);
}

//==============================================================================
ZsKeyBpmAudioProcessorEditor::ZsKeyBpmAudioProcessorEditor (ZsKeyBpmAudioProcessor& p)
    : AudioProcessorEditor (&p), plugin (p), content (p)
{
    setLookAndFeel (&lookAndFeel);
    addAndMakeVisible (content);

    setResizable (true, true);

    if (auto* constrainer = getConstrainer())
    {
        constrainer->setFixedAspectRatio ((double) zs::layout::width / (double) zs::layout::height);
        constrainer->setSizeLimits (roundToInt (zs::layout::width  * 0.78),
                                    roundToInt (zs::layout::height * 0.78),
                                    roundToInt (zs::layout::width  * 1.9),
                                    roundToInt (zs::layout::height * 1.9));
    }

    const auto storedWidth = (int) plugin.apvts.state.getProperty (
        ZsKeyBpmAudioProcessor::editorWidthProperty, zs::layout::width);

    const auto startWidth = jlimit (roundToInt (zs::layout::width * 0.78),
                                    roundToInt (zs::layout::width * 1.9),
                                    storedWidth);

    setSize (startWidth,
             roundToInt ((double) startWidth * zs::layout::height / zs::layout::width));
}

ZsKeyBpmAudioProcessorEditor::~ZsKeyBpmAudioProcessorEditor()
{
    setLookAndFeel (nullptr);
}

void ZsKeyBpmAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (zs::theme::background);
}

void ZsKeyBpmAudioProcessorEditor::resized()
{
    const auto scale = (float) getWidth() / (float) zs::layout::width;

    content.setBounds (0, 0, zs::layout::width, zs::layout::height);
    content.setTransform (AffineTransform::scale (scale));

    plugin.apvts.state.setProperty (ZsKeyBpmAudioProcessor::editorWidthProperty,
                                    getWidth(), nullptr);
}
