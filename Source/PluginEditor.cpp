#include "PluginEditor.h"
#include "BinaryData.h"
#include "Presets.h"

namespace
{
    using escoita::U;
    using escoita::numero;
}

namespace ui
{
// ================================================================ tipografía
// Montserrat (títulos e rótulos, en maiúsculas espaciadas coma na web de Escoita) e Inter
// (texto corrido): as da familia Barullo. As dúas, licenza OFL.
static juce::Typeface::Ptr tf (int which)
{
    static juce::Typeface::Ptr t[3] = {
        juce::Typeface::createSystemTypefaceFor (BinaryData::InterMedium_ttf, BinaryData::InterMedium_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratSemiBold_ttf, BinaryData::MontserratSemiBold_ttfSize),
        juce::Typeface::createSystemTypefaceFor (BinaryData::MontserratBold_ttf, BinaryData::MontserratBold_ttfSize) };
    return t[which];
}
juce::Font body (float s) { return juce::Font (juce::FontOptions (tf (0)).withHeight (s)); }
juce::Font semi (float s) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s)); }
juce::Font cond (float s) { return juce::Font (juce::FontOptions (tf (2)).withHeight (s)); }
juce::Font caps (float s, float tracking) { return juce::Font (juce::FontOptions (tf (1)).withHeight (s).withKerningFactor (tracking)); }
juce::String upper (const juce::String& t) { return escoita::maiusculas (t); }

void drawCaps (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, float size, float tracking)
{
    g.setFont (caps (size, tracking));
    g.drawText (upper (t), r, j, false);
}

void glowText (juce::Graphics& g, const juce::String& t, juce::Rectangle<float> r, juce::Justification j, juce::Colour c)
{
    g.setColour (c.withAlpha (0.16f));
    for (const auto d : { juce::Point<float> (-0.8f, 0.0f), { 0.8f, 0.0f }, { 0.0f, -0.8f }, { 0.0f, 0.8f } })
        g.drawText (t, r.translated (d.x, d.y), j, false);
    g.setColour (c);
    g.drawText (t, r, j, false);
}

// ================================================================ pezas do aparello
void paintPanel (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.11f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f * (float) i).expanded ((float) i), rad + (float) i);
    }
    g.setGradientFill (juce::ColourGradient (col::panel1, 0.0f, r.getY(), col::panel2, 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, rad);
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    g.fillRect (r.getX() + rad, r.getY() + 0.5f, r.getWidth() - 2.0f * rad, 1.0f);
}

void paintWell (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
    g.setColour (col::visor.brighter (0.06f));
    g.fillRoundedRectangle (r, rad);
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.65f), 0.0f, r.getY(), juce::Colours::transparentBlack, 0.0f, r.getY() + 5.0f, false));
    g.fillRect (r.withHeight (6.0f));
    g.restoreState();
}

void paintGlass (juce::Graphics& g, juce::Rectangle<float> r, float rad)
{
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.fillRoundedRectangle (r.translated (0.0f, 1.0f), rad);
    juce::ColourGradient gr (col::cristal1, r.getCentreX(), r.getY(), col::cristal2, r.getCentreX(), r.getBottom(), false);
    gr.addColour (0.55, juce::Colour (0xff0a1129));
    g.setGradientFill (gr);
    g.fillRoundedRectangle (r, rad);
    juce::Path p; p.addRoundedRectangle (r, rad);
    g.saveState();
    g.reduceClipRegion (p);
    for (int i = 0; i < 5; ++i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.16f - 0.03f * (float) i));
        g.drawRoundedRectangle (r.reduced ((float) i + 0.5f), rad, 1.0f);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), r.getX(), r.getY(),
                                             juce::Colours::transparentWhite, r.getX() + r.getWidth() * 0.32f, r.getY() + r.getHeight() * 0.55f, false));
    g.fillRect (r);
    g.restoreState();
}

void paintKey (juce::Graphics& g, juce::Rectangle<float> r, bool over, bool down)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 7.0f);
    if (down) r.translate (0.0f, 1.0f);
    const float lift = over && ! down ? 0.10f : 0.0f;
    g.setGradientFill (juce::ColourGradient (col::tecla1.brighter (lift), 0.0f, r.getY(), col::tecla2.brighter (lift * 0.6f), 0.0f, r.getBottom(), false));
    g.fillRoundedRectangle (r, 7.0f);
    g.setColour (juce::Colours::white.withAlpha (down ? 0.04f : 0.10f));
    g.fillRect (r.getX() + 6.0f, r.getY() + 0.5f, r.getWidth() - 12.0f, 1.0f);
}

void paintLed (juce::Graphics& g, juce::Point<float> c, bool on, juce::Colour luz)
{
    if (on)
    {
        g.setColour (luz.withAlpha (0.16f)); g.fillEllipse (c.x - 7.0f, c.y - 7.0f, 14.0f, 14.0f);
        g.setColour (luz.withAlpha (0.34f)); g.fillEllipse (c.x - 4.6f, c.y - 4.6f, 9.2f, 9.2f);
        g.setColour (luz); g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f)); g.fillEllipse (c.x - 1.3f, c.y - 2.0f, 2.2f, 2.0f);
    }
    else
    {
        g.setColour (juce::Colour (0xff3a3326)); g.fillEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f);
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawEllipse (c.x - 3.0f, c.y - 3.0f, 6.0f, 6.0f, 0.8f);
    }
}

// rótulo dun grupo: na súa cor e cunha liña debaixo
void groupTitle (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& t, juce::Colour c)
{
    g.setColour (c);
    drawCaps (g, t, r.withHeight (14.0f), juce::Justification::centred, 9.0f, 0.28f);
    g.setColour (c.withAlpha (0.25f));
    g.fillRect (r.getX() + 8.0f, r.getY() + 18.0f, r.getWidth() - 16.0f, 1.0f);
}

// ================================================================ aspecto
Look::Look()
{
    setColour (juce::ResizableWindow::backgroundColourId, col::noite);
    setColour (juce::Label::textColourId, col::crema);
    setColour (juce::TextEditor::backgroundColourId, col::visor);
    setColour (juce::TextEditor::textColourId, col::ouro);
    setColour (juce::TextEditor::outlineColourId, col::lina);
    setColour (juce::TextEditor::focusedOutlineColourId, col::laranxa);
    setColour (juce::PopupMenu::backgroundColourId, col::panel2);
    setColour (juce::PopupMenu::textColourId, col::crema);
    setColour (juce::PopupMenu::headerTextColourId, col::laranxa);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, col::laranxa);
    setColour (juce::PopupMenu::highlightedTextColourId, col::noite);
    setColour (juce::TextButton::buttonColourId, col::tecla2);
    setColour (juce::TextButton::textColourOffId, col::crema2);
    setColour (juce::TextButton::textColourOnId, col::crema);
    setColour (juce::AlertWindow::backgroundColourId, col::panel2);
    setColour (juce::AlertWindow::textColourId, col::crema);
    setColour (juce::AlertWindow::outlineColourId, col::lina);
}

juce::Typeface::Ptr Look::getTypefaceForFont (const juce::Font& f) { return tf (f.isBold() ? 1 : 0); }

// as teclas: en relevo, cun LED (aceso se está escollida; «led» = a cor da capa) e o texto en
// maiúsculas espaciadas; «gold» = un visor dourado (o selector da combinación)
void Look::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (1.5f);
    if (b.getProperties().contains ("gold"))
    {
        paintWell (g, r, 8.0f);
        if (over) { g.setColour (col::laranxa.withAlpha (0.08f)); g.fillRoundedRectangle (r, 8.0f); }
        return;
    }
    paintKey (g, r, over, down);
    if (down) r.translate (0.0f, 1.0f);
    if (! b.getProperties().contains ("noLed"))
    {
        const auto luz = b.getProperties().contains ("led") ? juce::Colour ((juce::uint32) (juce::int64) b.getProperties()["led"]) : col::laranxa;
        paintLed (g, { r.getX() + 12.0f, r.getCentreY() }, b.getToggleState(), luz);
    }
}

void Look::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f).withTrimmedBottom (1.5f);
    if (b.getProperties().contains ("gold"))
    {
        g.setFont (caps (12.0f, 0.1f));
        glowText (g, upper (b.getButtonText()), r.reduced (14.0f, 0.0f).withTrimmedRight (16.0f), juce::Justification::centredLeft, col::ouro);
        juce::Path arrow;
        const float ax = r.getRight() - 18.0f, ay = r.getCentreY();
        arrow.addTriangle (ax - 5.0f, ay - 2.5f, ax + 5.0f, ay - 2.5f, ax, ay + 3.5f);
        g.setColour (col::ouro); g.fillPath (arrow);
        return;
    }
    if (down) r.translate (0.0f, 1.0f);
    if (! b.getProperties().contains ("noLed")) r.removeFromLeft (14.0f);
    juce::Colour c = b.getToggleState() || over ? col::crema : col::crema2;
    if (! b.isEnabled()) c = col::crema3.withAlpha (0.6f);
    g.setColour (c);
    const int curve = b.getProperties()["curve"];   // desfacer (−1) e refacer (+1): unha frecha en arco
    if (curve != 0)
    {
        const auto ctr = r.getCentre(); const float rad = 6.5f;
        const float a0 = curve > 0 ? -2.4f : 2.4f, a1 = curve > 0 ? 1.2f : -1.2f;
        juce::Path arc; arc.addCentredArc (ctr.x, ctr.y + 1.0f, rad, rad, 0.0f, a0, a1, true);
        g.strokePath (arc, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        const juce::Point<float> tip (ctr.x + rad * std::sin (a1), ctr.y + 1.0f - rad * std::cos (a1));
        juce::Path head; head.addTriangle (tip.x + (float) curve * 4.0f, tip.y, tip.x - (float) curve * 1.5f, tip.y - 4.0f, tip.x - (float) curve * 1.5f, tip.y + 4.0f);
        g.fillPath (head);
        return;
    }
    g.setFont (caps ((float) b.getProperties().getWithDefault ("fontSize", 9.5f), 0.15f));
    g.drawFittedText (upper (b.getButtonText()), r.toNearestInt().reduced (3, 0), juce::Justification::centred, 1, 0.7f);
}

// os mandos: disco mecanizado, anel de LED laranxa e o valor nun visor debaixo
void Look::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, juce::Slider& s)
{
    auto area = juce::Rectangle<int> (x, y, w, h).toFloat();
    auto lcd = area.removeFromBottom (17.0f);
    lcd = lcd.withSizeKeepingCentre (juce::jmin (66.0f, area.getWidth() - 2.0f), 17.0f);
    area.removeFromBottom (3.0f);
    const float d = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.withSizeKeepingCentre (d, d).getCentre();
    const float ring = d * 0.5f - 4.0f;
    const auto luz = s.getProperties().contains ("led") ? juce::Colour ((juce::uint32) (juce::int64) s.getProperties()["led"]) : col::laranxa;

    juce::Path track; track.addCentredArc (c.x, c.y, ring, ring, 0.0f, a0, a1, true);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    const bool bip = s.getMinimum() < 0 && s.getMaximum() > 0;
    const double zeroAt = bip ? 0.0 : s.getMinimum();
    const float zero = a0 + (float) s.valueToProportionOfLength (zeroAt) * (a1 - a0);
    const float ang = a0 + pos * (a1 - a0);
    if (std::abs (ang - zero) > 0.01f)
    {
        juce::Path val; val.addCentredArc (c.x, c.y, ring, ring, 0.0f, juce::jmin (zero, ang), juce::jmax (zero, ang), true);
        const auto lit = s.isEnabled() ? luz : col::crema3;
        g.setColour (lit.withAlpha (0.12f)); g.strokePath (val, juce::PathStrokeType (8.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (lit.withAlpha (0.26f)); g.strokePath (val, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (lit); g.strokePath (val, juce::PathStrokeType (2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    const float kr = ring - 7.0f;
    g.setColour (juce::Colours::black.withAlpha (0.45f));
    g.fillEllipse (c.x - kr, c.y - kr + 2.5f, kr * 2.0f, kr * 2.0f);
    juce::ColourGradient disc (juce::Colour (0xff4a5577), c.x - kr * 0.3f, c.y - kr * 0.45f, juce::Colour (0xff0e1323), c.x + kr * 0.55f, c.y + kr * 1.05f, true);
    disc.addColour (0.55, juce::Colour (0xff232b45));
    g.setGradientFill (disc);
    g.fillEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);
    const juce::Point<float> tip (c.x + kr * 0.82f * std::sin (ang), c.y - kr * 0.82f * std::cos (ang));
    g.setColour (col::crema.withAlpha (s.isEnabled() ? 0.92f : 0.4f));
    g.drawLine ({ c, tip }, 2.0f);

    paintWell (g, lcd, 5.0f);
    g.setFont (caps (9.0f, 0.04f));
    glowText (g, upper (s.getTextFromValue (s.getValue())), lcd, juce::Justification::centred, s.isEnabled() ? col::ouro : col::crema3);
}

// os deslizadores: a pista afundida, a luz laranxa desde abaixo ata o capuchón e o capuchón mecanizado
void Look::drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    const float cx = (float) x + (float) w * 0.5f;
    const float top = (float) y, bot = (float) (y + h);
    auto slot = juce::Rectangle<float> (cx - 4.5f, top - 7.0f, 9.0f, bot - top + 14.0f);
    paintWell (g, slot, 4.5f);
    const int steps = s.getMaximum() - s.getMinimum() <= 3.5 ? (int) (s.getMaximum() - s.getMinimum()) : 10;
    g.setColour (col::crema3.withAlpha (0.55f));
    for (int t = 0; t <= steps; ++t)
    {
        const float ty = bot - (bot - top) * (float) t / (float) juce::jmax (1, steps);
        const float len = (t % 5 == 0 || steps <= 3) ? 5.0f : 2.5f;
        g.fillRect (cx - 9.0f - len, ty - 0.5f, len, 1.0f);
        g.fillRect (cx + 9.0f, ty - 0.5f, len, 1.0f);
    }
    if (pos < bot - 0.5f)
    {
        auto lit = juce::Rectangle<float> (cx - 1.5f, pos, 3.0f, bot + 4.0f - pos);
        g.setColour (col::laranxa.withAlpha (0.14f)); g.fillRoundedRectangle (lit.expanded (3.0f, 0.0f), 3.0f);
        g.setColour (col::laranxa.withAlpha (0.30f)); g.fillRoundedRectangle (lit.expanded (1.5f, 0.0f), 2.0f);
        g.setColour (col::laranxa); g.fillRoundedRectangle (lit, 1.5f);
    }
    auto cap = juce::Rectangle<float> (cx - 13.0f, pos - 7.0f, 26.0f, 14.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (cap.translated (0.0f, 2.5f), 4.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a5577), cap.getX(), cap.getY(), juce::Colour (0xff141a2e), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.drawRoundedRectangle (cap.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (col::crema); g.fillRect (cap.getX() + 4.0f, pos - 0.8f, cap.getWidth() - 8.0f, 1.6f);
}

void Look::drawTooltip (juce::Graphics& g, const juce::String& text, int w, int h)
{
    auto r = juce::Rectangle<int> (w, h).toFloat();
    g.setColour (col::panel2); g.fillRoundedRectangle (r, 6.0f);
    g.setColour (col::lina); g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 1.0f);
    juce::AttributedString s; s.setJustification (juce::Justification::topLeft);
    s.append (text, body (13.5f), col::crema);
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    tl.draw (g, r.reduced (10.0f, 7.0f));
}

juce::Rectangle<int> Look::getTooltipBounds (const juce::String& text, juce::Point<int> pos, juce::Rectangle<int> parent)
{
    juce::AttributedString s; s.append (text, body (13.5f));
    juce::TextLayout tl; tl.createLayoutWithBalancedLineLengths (s, 300.0f);
    const int w = (int) tl.getWidth() + 22, h = (int) tl.getHeight() + 16;
    return juce::Rectangle<int> (pos.x > parent.getCentreX() ? pos.x - (w + 12) : pos.x + 12,
                                 pos.y > parent.getCentreY() ? pos.y - (h + 8) : pos.y + 18, w, h).constrainedWithin (parent);
}

void Look::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (col::panel2);
    g.setColour (col::lina); g.drawRect (0, 0, w, h);
}

// ================================================================ mando
Knob::Knob()
{
    slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f, juce::MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (180);
    addAndMakeVisible (slider);
}

void Knob::paint (juce::Graphics& g)
{
    g.setColour (slider.isEnabled() ? col::crema2 : col::crema3);
    drawCaps (g, name, getLocalBounds().removeFromBottom (12).toFloat(), juce::Justification::centred, 7.5f, 0.16f);
}



// ================================================================ a pantalla
static juce::String tempo (double s) { return s < 0.995 ? juce::String (juce::roundToInt (s * 1000.0)) + " ms" : numero (s, s < 9.95 ? 1 : 0) + " s"; }

void Pantalla::tick()
{
    auto caer = [] (float& v, float novo) { v = juce::jmax (novo, v * 0.88f); };
    caer (entrada, proc.nivelEntrada.load()); caer (cola, proc.nivelCola.load()); caer (saida, proc.nivelSaida.load());
    fase += 0.02f;
    repaint();
}

void Pantalla::paint (juce::Graphics& g)
{
    const auto r = getLocalBounds().toFloat();
    paintGlass (g, r, 12.0f);
    const auto a = proc.axustes();
    static const char* tipos[verb::NUM_TIPOS] = { "Cuarto", "Sala", "Placa", "Igrexa", "Shimmer" };

    auto in = r.reduced (18.0f, 14.0f);
    auto meters = in.removeFromRight (96.0f);
    in.removeFromRight (14.0f);
    // o rótulo: o tipo e os tempos
    {
        auto top = in.removeFromTop (14.0f);
        g.setColour (col::crema3);
        drawCaps (g, U ("Caída · ") + U (tipos[juce::jlimit (0, verb::NUM_TIPOS - 1, a.tipo)]), top, juce::Justification::centredLeft, 8.0f, 0.24f);
        drawCaps (g, U ("Predelay ") + juce::String (juce::roundToInt (a.predelay)) + " ms", top, juce::Justification::centredRight, 8.0f, 0.24f);
    }
    in.removeFromTop (6.0f);
    auto graf = in.withTrimmedBottom (14.0f);
    const double pre = a.predelay / 1000.0;
    const double tG = verb::Espazo::tempoBanda (a, 0), tM = verb::Espazo::tempoBanda (a, 1), tA = verb::Espazo::tempoBanda (a, 2);
    const double tmax = juce::jmax (0.5, (pre + juce::jmax (tG, tM, tA)) * 1.08);
    auto xOf = [&] (double t) { return graf.getX() + (float) (t / tmax) * graf.getWidth(); };
    auto yOf = [&] (double db) { return graf.getY() + (float) (-db / 60.0) * graf.getHeight(); };

    // a reixa: cada 10 dB e unhas marcas de tempo
    g.setColour (col::lina.withAlpha (0.5f));
    for (int db = 0; db >= -60; db -= 10) g.fillRect (graf.getX(), yOf (db), graf.getWidth(), 1.0f);
    const double paso = tmax > 12 ? 2.0 : tmax > 5 ? 1.0 : tmax > 2 ? 0.5 : tmax > 0.8 ? 0.2 : 0.1;
    g.setFont (caps (7.0f, 0.06f));
    for (double t = 0; t <= tmax + 1e-9; t += paso)
    {
        g.setColour (col::lina.withAlpha (0.5f));
        g.fillRect (xOf (t), graf.getY(), 1.0f, graf.getHeight());
        g.setColour (col::crema3);
        g.drawText (tempo (t), juce::Rectangle<float> (xOf (t) + 3.0f, graf.getBottom() + 2.0f, 60.0f, 11.0f), juce::Justification::centredLeft, false);
    }
    // o predelay
    if (pre > 0) { g.setColour (col::azul.withAlpha (0.08f)); g.fillRect (graf.withRight (xOf (pre))); }

    // as reflexións
    for (int k = 0; k < verb::Espazo::numReflexions(); ++k)
    {
        const double nivel = verb::Espazo::reflexionNivel (a, k);
        if (nivel <= 0.001) continue;
        const float x = xOf (pre + verb::Espazo::reflexionMs (a, k) / 1000.0);
        const float y = yOf (juce::jmax (-60.0, 20.0 * std::log10 (nivel) - 3.0));
        g.setColour (col::crema.withAlpha (0.55f));
        g.fillRect (x, y, 1.5f, graf.getBottom() - y);
    }

    // as tres bandas: cada unha cae ata −60 dB no seu tempo (ou queda, se está conxelada)
    auto liña = [&] (double t, juce::Colour c, float grosor, bool encher)
    {
        juce::Path p;
        const float x0 = xOf (pre), y0 = yOf (0);
        const float x1 = a.conxelar ? graf.getRight() : xOf (pre + t), y1 = a.conxelar ? y0 : yOf (-60);
        p.startNewSubPath (x0, y0); p.lineTo (x1, y1);
        if (encher)
        {
            juce::Path f (p); f.lineTo (x1, graf.getBottom()); f.lineTo (x0, graf.getBottom()); f.closeSubPath();
            g.setGradientFill (juce::ColourGradient (c.withAlpha (0.28f), x0, y0, c.withAlpha (0.02f), x0, graf.getBottom(), false));
            g.fillPath (f);
        }
        g.setColour (c.withAlpha (0.25f)); g.strokePath (p, juce::PathStrokeType (grosor + 4.0f));
        g.setColour (c); g.strokePath (p, juce::PathStrokeType (grosor));
    };
    if (! a.conxelar)
    {
        liña (tG, col::verde, 1.4f, false);
        liña (tA, col::ouro, 1.4f, false);
    }
    liña (tM, col::laranxa, 2.4f, true);

    // a lenda: o tempo de cada banda (ou «conxelada»)
    {
        struct Item { juce::String nome; double t; juce::Colour c; };
        std::vector<Item> items;
        if (a.conxelar) items.push_back ({ U ("Conxelada: non se apaga"), -1, col::laranxa });
        else items = { { "Graves", tG, col::verde }, { "Medios", tM, col::laranxa }, { "Agudos", tA, col::ouro } };
        const auto fonte = caps (8.0f, 0.16f);
        float ancho = 0;
        std::vector<juce::String> txt;
        for (auto& it : items)
        {
            txt.push_back (upper (it.nome) + (it.t >= 0 ? "  " + tempo (it.t) : juce::String()));
            ancho += 22.0f + juce::GlyphArrangement::getStringWidth (fonte, txt.back()) + 18.0f;
        }
        float x = graf.getCentreX() - ancho * 0.5f;
        const float y = r.getY() + 14.0f;
        g.setFont (fonte);
        for (size_t i = 0; i < items.size(); ++i)
        {
            g.setColour (items[i].c);
            g.fillRoundedRectangle (x, y + 5.0f, 16.0f, 3.0f, 1.5f);
            g.setColour (col::crema2);
            const float w = juce::GlyphArrangement::getStringWidth (fonte, txt[i]);
            g.drawText (txt[i], juce::Rectangle<float> (x + 22.0f, y, w + 4.0f, 14.0f), juce::Justification::centredLeft, false);
            x += 22.0f + w + 18.0f;
        }
    }

    // o shimmer: unhas faíscas que soben
    if (a.shimmer > 0.01)
    {
        juce::Random rnd (7);
        for (int i = 0; i < 18; ++i)
        {
            const float x = graf.getX() + rnd.nextFloat() * graf.getWidth();
            const float t = std::fmod (fase * (0.3f + rnd.nextFloat()) + rnd.nextFloat(), 1.0f);
            const float y = graf.getBottom() - t * graf.getHeight();
            g.setColour (col::ouro.withAlpha ((float) a.shimmer * (1.0f - t) * 0.8f));
            g.fillEllipse (x - 1.5f, y - 1.5f, 3.0f, 3.0f);
        }
    }

    // os niveis: entrada, cola e saída
    const juce::String nomes[3] = { "Entrada", "Cola", U ("Saída") };
    const float niveis[3] = { entrada, cola, saida };
    const float mw = (meters.getWidth() - 12.0f) / 3.0f;
    for (int i = 0; i < 3; ++i)
    {
        auto m = juce::Rectangle<float> (meters.getX() + (float) i * (mw + 6.0f), meters.getY(), mw, meters.getHeight() - 14.0f);
        paintWell (g, m, 4.0f);
        const float db = juce::Decibels::gainToDecibels (niveis[i], -60.0f);
        const float alto = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f) * (m.getHeight() - 4.0f);
        if (alto > 0)
        {
            auto bar = m.reduced (3.0f, 2.0f).removeFromBottom (alto);
            g.setGradientFill (juce::ColourGradient (i == 1 ? col::laranxa : col::verde, 0.0f, m.getBottom(), col::ouro, 0.0f, m.getY(), false));
            g.fillRoundedRectangle (bar, 2.0f);
        }
        g.setColour (col::crema3);
        drawCaps (g, nomes[i], juce::Rectangle<float> (m.getX() - 6.0f, m.getBottom() + 2.0f, m.getWidth() + 12.0f, 10.0f), juce::Justification::centred, 6.5f, 0.06f);
    }
}

// ================================================================ vista principal
MainView::MainView (BVProcessor& p) : proc (p), pantalla (p)
{
    setLookAndFeel (&look);
    auto key = [this] (juce::TextButton& b, const juce::String& tip) { b.setTooltip (tip); b.setWantsKeyboardFocus (false); addAndMakeVisible (b); };

    // cabeceira: o preset, os presets propios, o A/B e desfacer
    presetBtn.getProperties().set ("gold", true);
    presetBtn.onClick = [this] { presetsMenu(); };
    key (presetBtn, U ("Os presets de fábrica, por grupos: cuartos, salas, placas, igrexas e shimmer."));
    prevBtn.setButtonText (U ("‹")); nextBtn.setButtonText (U ("›"));
    for (auto* b : { &prevBtn, &nextBtn, &presetsBtn, &undoBtn, &redoBtn, &funcBtn }) b->getProperties().set ("noLed", true);
    prevBtn.onClick = [this] { proc.estado.novoPaso(); proc.cargarPreset ((proc.getPreset() + verb::NUM_PRESETS - 1) % verb::NUM_PRESETS); refreshAll(); };
    nextBtn.onClick = [this] { proc.estado.novoPaso(); proc.cargarPreset ((proc.getPreset() + 1) % verb::NUM_PRESETS); refreshAll(); };
    key (prevBtn, U ("O preset anterior."));
    key (nextBtn, U ("O preset seguinte."));
    presetsBtn.onClick = [this] { showPresetsMenu(); };
    key (presetsBtn, U ("Os teus presets: gardar o actual, abrir a carpeta e copiar entre A e B."));
    for (int i = 0; i < 2; ++i)
    {
        abBtn[i].setButtonText (i == 0 ? "A" : "B");
        abBtn[i].onClick = [this, i] { if (i != proc.estado.lado()) proc.estado.cambiarLado(); refreshAll(); };
        key (abBtn[i], U ("A/B: dous axustes para comparar. Cambiar de lado garda o actual; para copiar un lado no outro, no menú dos presets."));
    }
    undoBtn.getProperties().set ("curve", -1); redoBtn.getProperties().set ("curve", 1);
    undoBtn.setTitle (U ("Desfacer")); redoBtn.setTitle (U ("Refacer"));
    undoBtn.onClick = [this] { proc.undo.undo(); refreshAll(); };
    redoBtn.onClick = [this] { proc.undo.redo(); refreshAll(); };
    key (undoBtn, U ("Desfacer o último cambio. Cada clic é un paso."));
    key (redoBtn, U ("Refacer."));
    funcBtn.setButtonText (U ("Función"));
    funcBtn.onClick = [this] { functionMenu(); };
    key (funcBtn, U ("Avisos e ligazóns."));

    // o tipo de espazo
    {
        auto* prm = proc.apvts.getParameter ("tipo");
        tipoAtt = std::make_unique<juce::ParameterAttachment> (*prm, [this, prm] (float v)
        {
            const int t = (int) std::lround (prm->convertFrom0to1 (prm->convertTo0to1 (v)));
            for (int i = 0; i < verb::NUM_TIPOS; ++i) tipoBtn[i].setToggleState (i == t, juce::dontSendNotification);
            pantalla.repaint();
        });
        const char* nomes[verb::NUM_TIPOS] = { "Cuarto", "Sala", "Placa", "Igrexa", "Shimmer" };
        const juce::String axudas[verb::NUM_TIPOS] = {
            U ("Cuarto: pequeno e vivo, coas reflexións preto."), U ("Sala: o equilibrio, para case todo."),
            U ("Placa: densa e brillante, sen reflexións. Para voces e caixas."), U ("Igrexa: grande, de pedra; os agudos apáganse antes."),
            U ("Shimmer: unha sala onde a cola sobe unha oitava (canto, co mando Shimmer, en Máis).") };
        for (int i = 0; i < verb::NUM_TIPOS; ++i)
        {
            tipoBtn[i].setButtonText (nomes[i]);
            tipoBtn[i].onClick = [this, i]
            {
                tipoAtt->setValueAsCompleteGesture ((float) i);
                // o tipo Shimmer co shimmer a cero soaría coma unha sala: dálle un pouco
                auto* sh = proc.apvts.getParameter ("shimmer");
                if (i == verb::SHIMMER && sh->getValue() < 0.001f)
                {
                    sh->beginChangeGesture(); sh->setValueNotifyingHost (sh->convertTo0to1 (45.0f)); sh->endChangeGesture();
                }
            };
            key (tipoBtn[i], axudas[i]);
        }
        tipoAtt->sendInitialUpdate();
    }
    {
        auto* prm = proc.apvts.getParameter ("conxelar");
        conxelarAtt = std::make_unique<juce::ParameterAttachment> (*prm, [this] (float v) { conxelarBtn.setToggleState (v > 0.5f, juce::dontSendNotification); pantalla.repaint(); });
        conxelarAtt->sendInitialUpdate();
        conxelarBtn.onClick = [this] { conxelarAtt->setValueAsCompleteGesture (conxelarBtn.getToggleState() ? 0.0f : 1.0f); };
        key (conxelarBtn, U ("Conxelar: a cola queda soando sen fin e xa non entra nada novo. Para fondos e transicións."));
    }
    maisBtn.setButtonText (U ("Máis"));
    maisBtn.onClick = [this] { mostrarMais (! mais); };
    key (maisBtn, U ("Os demais controis: difusión, movemento, graves, reflexións, anchura, cortes, shimmer e atenuación."));

    auto knob = [this] (Knob& kn, const char* pid, const juce::String& name, const juce::String& tip, std::function<juce::String (double)> fmt)
    {
        kn.name = name;
        kn.slider.setTooltip (tip);
        kn.att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, pid, kn.slider);
        kn.slider.textFromValueFunction = std::move (fmt);
        kn.slider.onValueChange = [this] { pantalla.repaint(); };
        addAndMakeVisible (kn);
    };
    auto pc = [] (double v) { return juce::String (juce::roundToInt (v)) + " %"; };
    auto hz = [] (double v) { return v < 999.5 ? juce::String (juce::roundToInt (v)) + " Hz" : numero (v / 1000.0, v < 9950 ? 1 : 0) + " kHz"; };
    knob (caida, "caida", U ("Tamaño"), U ("Canto tarda a reverb en apagarse (60 dB, nos medios)."), [] (double v) { return tempo (v); });
    knob (predelay, "predelay", "Predelay", U ("O tempo antes de que chegue a reverb: separa a voz da cola."), [] (double v) { return juce::String (juce::roundToInt (v)) + " ms"; });
    knob (brillo, "brillo", "Brillo", U ("Canto aguantan os agudos: escuro (cortinas) ou brillante (pedra, placa)."), pc);
    knob (mestura, "mestura", "Mestura", U ("Canto se oe a reverb fronte ao son seco."), pc);
    knob (difusion, "difusion", U ("Difusión"), U ("Canto se esparexe o ataque: baixa, ecos que se oen; alta, unha néboa lisa."), pc);
    knob (modulacion, "modulacion", "Movemento", U ("A cola móvese un chisco (coma un chorus): máis viva e sen timbre metálico."), pc);
    knob (graves, "graves", "Graves", U ("O tempo dos graves fronte aos medios: menos de ×1, máis limpa; máis, máis cálida."), [] (double v) { return U ("×") + numero (v, 2); });
    knob (reflexions, "reflexions", U ("Reflexións"), U ("As primeiras reflexións, as das paredes máis preto: dan o tamaño do sitio."), pc);
    knob (anchura, "anchura", "Anchura", U ("A anchura estéreo da reverb: 0 % mono, 100 % normal, 150 % moi aberta."), pc);
    knob (corteG, "corteGraves", "Corte graves", U ("Quita os graves da reverb (non do son seco) para que non embarre."), hz);
    knob (corteA, "corteAgudos", "Corte agudos", U ("Quita os agudos da reverb."), hz);
    knob (shimmer, "shimmer", "Shimmer", U ("A cola sobe unha oitava e volve entrar: brillo que flota (en calquera tipo)."), pc);
    knob (atenuacion, "atenuacion", U ("Atenuación"), U ("A reverb baixa mentres soa o son seco e volve cando para: claridade sen perder cola."), pc);
    knob (volume, "volume", "Volume", U ("O volume de saída (despois, o limitador a −1 dBFS)."), [] (double v) { return (v >= 0.05 ? "+" : "") + numero (v, 1) + " dB"; });
    volume.slider.setDoubleClickReturnValue (true, 0.0);
    for (auto* k : { &caida, &predelay, &brillo, &mestura }) k->slider.setMouseDragSensitivity (220);

    addAndMakeVisible (pantalla);
    newVersion.getProperties().set ("noLed", true);
    newVersion.onClick = [] { juce::URL (escoita::Actualizacions::downloadsUrl()).launchInDefaultBrowser(); };
    newVersion.setWantsKeyboardFocus (false);
    addChildComponent (newVersion);

    juce::Desktop::getInstance().addGlobalMouseListener (this);
    setSize (W, H);
    mostrarMais (false);
    refreshAll();
    startTimerHz (30);
}

MainView::~MainView()
{
    juce::Desktop::getInstance().removeGlobalMouseListener (this);
    stopTimer();
    setLookAndFeel (nullptr);
}

// cada clic (en calquera parte) empeza un paso novo de desfacer
void MainView::mouseDown (const juce::MouseEvent&) { proc.estado.novoPaso(); }

void MainView::mostrarMais (bool s)
{
    mais = s;
    maisBtn.setToggleState (s, juce::dontSendNotification);
    pantalla.setVisible (! s);
    for (auto* k : { &difusion, &modulacion, &graves, &reflexions, &anchura, &corteG, &corteA, &shimmer, &atenuacion, &volume }) k->setVisible (s);
    repaint();
}

void MainView::presetsMenu()
{
    juce::PopupMenu m;
    juce::String grupo;
    for (int i = 0; i < verb::NUM_PRESETS; ++i)
    {
        if (grupo != verb::PRESETS[i].grupo) { grupo = verb::PRESETS[i].grupo; m.addSectionHeader (U (verb::PRESETS[i].grupo)); }
        m.addItem (100 + i, U (verb::PRESETS[i].nome), true, i == proc.getPreset() && proc.nomePreset() == U (verb::PRESETS[i].nome));
    }
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetBtn), [this] (int r)
    {
        if (r < 100) return;
        proc.estado.novoPaso();
        proc.cargarPreset (r - 100);
        refreshAll();
    });
}

void MainView::showPresetsMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader (U ("Os teus presets"));
    proc.estado.engadirAoMenu (m, [sp = juce::Component::SafePointer<MainView> (this)] { if (sp != nullptr) sp->refreshAll(); });
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&presetsBtn));
}

void MainView::functionMenu()
{
    juce::PopupMenu m;
    m.addSectionHeader (U ("Función"));
    m.addItem (6, U ("Avisar das versións novas"), true, versions->enabled());
    m.addItem (7, U ("escoita.link"));
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&funcBtn), [this] (int r)
    {
        if (r == 6) versions->setEnabled (! versions->enabled());
        else if (r == 7) juce::URL ("https://www.escoita.link").launchInDefaultBrowser();
    });
}

void MainView::refreshState()
{
    for (int i = 0; i < 2; ++i)
        if (abBtn[i].getToggleState() != (i == proc.estado.lado())) abBtn[i].setToggleState (i == proc.estado.lado(), juce::dontSendNotification);
    if (undoBtn.isEnabled() != proc.undo.canUndo()) undoBtn.setEnabled (proc.undo.canUndo());
    if (redoBtn.isEnabled() != proc.undo.canRedo()) redoBtn.setEnabled (proc.undo.canRedo());
    const auto t = proc.nomePreset() + (proc.presetEditado() ? U (" *") : juce::String());
    if (presetBtn.getButtonText() != t) presetBtn.setButtonText (t);
}

void MainView::refreshAll()
{
    refreshState();
    pantalla.repaint();
}

void MainView::timerCallback()
{
    refreshState();
    if (! mais) pantalla.tick();
    const auto nv = versions->newVersion();
    if (nv.isNotEmpty() != newVersion.isVisible())
    {
        newVersion.setButtonText (U ("Hai unha versión nova: ") + nv + U (" · descargar"));
        newVersion.setVisible (nv.isNotEmpty());
        repaint();
    }
}

void MainView::paint (juce::Graphics& g)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff161e33), 0.0f, 0.0f, juce::Colour (0xff0b101e), 0.0f, (float) H, false));
    g.fillAll();

    paintPanel (g, headerArea.toFloat(), 14.0f);
    {
        const auto plate = juce::Rectangle<float> ((float) headerArea.getX() + 10.0f, (float) headerArea.getY() + 6.0f, 188.0f, (float) headerArea.getHeight() - 12.0f);
        paintWell (g, plate, 9.0f);
        g.setColour (col::crema);
        g.setFont (cond (14.5f).withExtraKerningFactor (0.07f));
        g.drawText ("BARULLO VERB", plate.reduced (14.0f, 0.0f).withTrimmedBottom (14.0f).withTrimmedTop (4.0f), juce::Justification::centredLeft, false);
        auto sub = plate.reduced (14.0f, 0.0f).withTop (plate.getBottom() - 17.0f).withHeight (11.0f);
        g.setFont (caps (7.5f, 0.36f));
        const float we = juce::GlyphArrangement::getStringWidth (caps (7.5f, 0.36f), "ESCOITA");
        g.setColour (col::crema2); g.drawText ("ESCOITA", sub, juce::Justification::centredLeft, false);
        g.setColour (col::laranxa); g.drawText (".", sub.withTrimmedLeft (we - 1.5f), juce::Justification::centredLeft, false);
        g.setColour (col::crema2); g.drawText (U (" · FERRAMENTAS"), sub.withTrimmedLeft (we + 4.0f), juce::Justification::centredLeft, false);
    }
    auto well = [&] (juce::Component& a, juce::Component& b) { paintWell (g, a.getBounds().getUnion (b.getBounds()).expanded (4).toFloat(), 11.0f); };
    well (prevBtn, nextBtn);
    well (presetsBtn, redoBtn);

    if (mais)
    {
        paintPanel (g, screenArea.toFloat(), 12.0f);
        auto titulo = [&] (juce::Component& a, juce::Component& b, const juce::String& t)
        { groupTitle (g, juce::Rectangle<float> ((float) a.getX() + 4.0f, (float) screenArea.getY() + 18.0f, (float) (b.getRight() - a.getX()) - 8.0f, 20.0f), t); };
        titulo (difusion, anchura, U ("O espazo"));
        titulo (corteG, volume, U ("O son da cola"));
    }

    paintPanel (g, controlsArea.toFloat(), 14.0f);
    paintWell (g, tipoBtn[0].getBounds().getUnion (tipoBtn[verb::NUM_TIPOS - 1].getBounds()).expanded (4).toFloat(), 11.0f);

    const auto r = juce::Rectangle<float> (22.0f, (float) H - 22.0f, (float) W - 44.0f, 14.0f);
    g.setColour (col::crema3);
    const float we = juce::GlyphArrangement::getStringWidth (caps (8.0f, 0.3f), "ESCOITA");
    drawCaps (g, "ESCOITA", r, juce::Justification::centredLeft, 8.0f, 0.3f);
    g.setColour (col::laranxa);
    drawCaps (g, ".", r.withTrimmedLeft (we - 2.0f), juce::Justification::centredLeft, 8.0f, 0.3f);
    g.setColour (col::crema3);
    drawCaps (g, U (" Laboratorio creativo · v") + escoita::Actualizacions::currentVersion(), r.withTrimmedLeft (we + 3.0f), juce::Justification::centredLeft, 8.0f, 0.3f);
    if (! newVersion.isVisible())
        drawCaps (g, U ("Unha reverb de Barullo"), r, juce::Justification::centred, 8.0f, 0.2f);
    drawCaps (g, U ("42°14′N 8°43′W"), r, juce::Justification::centredRight, 8.0f, 0.3f);
}

void MainView::resized()
{
    const int x0 = 16, w = W - 32;
    headerArea = { x0, 14, w, 52 };
    screenArea = { x0, 76, w, 240 };
    controlsArea = { x0, 326, w, 238 };
    {
        int x = headerArea.getX() + 210;
        const int y = headerArea.getY() + 10, h = 32;
        presetBtn.setBounds (x, y, 214, h); x += 226;
        prevBtn.setBounds (x, y, 34, h); x += 38;
        nextBtn.setBounds (x, y, 34, h); x += 48;
        presetsBtn.setBounds (x, y, 76, h); x += 80;
        abBtn[0].setBounds (x, y, 36, h); x += 40;
        abBtn[1].setBounds (x, y, 36, h); x += 44;
        undoBtn.setBounds (x, y, 32, h); x += 36;
        redoBtn.setBounds (x, y, 32, h);
        funcBtn.setBounds (headerArea.getRight() - 10 - 80, y, 80, h);
    }
    pantalla.setBounds (screenArea);
    {
        // «Máis»: dous grupos de cinco, nunha fila
        const int kw = 80, kh = 150, gap = 24;
        const int grupo = 5 * kw, x0g = screenArea.getCentreX() - grupo - gap / 2 - 6, y = screenArea.getY() + 54;
        int i = 0;
        for (auto* k : { &difusion, &modulacion, &graves, &reflexions, &anchura }) k->setBounds (x0g + i++ * kw, y, kw, kh);
        i = 0;
        for (auto* k : { &corteG, &corteA, &shimmer, &atenuacion, &volume }) k->setBounds (x0g + grupo + gap + 12 + i++ * kw, y, kw, kh);
    }
    {
        const int tw = 108, th = 30, ty = controlsArea.getY() + 16;
        const int tx = controlsArea.getCentreX() - (verb::NUM_TIPOS * tw + (verb::NUM_TIPOS - 1) * 4) / 2;
        for (int i = 0; i < verb::NUM_TIPOS; ++i) tipoBtn[i].setBounds (tx + i * (tw + 4), ty, tw, th);
        const int kw = 130, kh = 150, ky = controlsArea.getY() + 62;
        const int kx = controlsArea.getCentreX() - 2 * kw;
        int i = 0;
        for (auto* k : { &caida, &predelay, &brillo, &mestura }) k->setBounds (kx + i++ * kw, ky, kw, kh);
        conxelarBtn.setBounds (controlsArea.getX() + 22, ky + kh / 2 - 20, 118, 32);
        maisBtn.setBounds (controlsArea.getRight() - 22 - 118, ky + kh / 2 - 20, 118, 32);
    }
    newVersion.setBounds (W / 2 - 170, H - 27, 340, 22);
}
} // namespace ui

// ================================================================ xanela (escalable)
BVEditor::BVEditor (BVProcessor& p) : AudioProcessorEditor (&p), proc (p), main (p)
{
    addAndMakeVisible (main);
    setResizable (true, true);
    setResizeLimits (ui::MainView::W * 6 / 10, ui::MainView::H * 6 / 10, ui::MainView::W * 2, ui::MainView::H * 2);
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) ui::MainView::W / ui::MainView::H);
    float s = proc.uiScale;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
    {
        const auto ua = d->userArea;
        const float fit = juce::jmin ((float) (ua.getWidth() - 40) / (float) ui::MainView::W, (float) (ua.getHeight() - 90) / (float) ui::MainView::H);
        if (s > fit && fit > 0.4f) s = fit;
    }
    setSize (juce::roundToInt (ui::MainView::W * s), juce::roundToInt (ui::MainView::H * s));
}

void BVEditor::resized()
{
    const float s = (float) getWidth() / (float) ui::MainView::W;
    main.setBounds (0, 0, ui::MainView::W, ui::MainView::H);
    main.setTransform (juce::AffineTransform::scale (s));
    proc.uiScale = s;
}
