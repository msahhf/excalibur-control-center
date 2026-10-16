import { Nav } from "./components/Nav";
import { Hero } from "./components/Hero";
import { Story } from "./components/Story";
import { ReadOnly } from "./components/ReadOnly";
import { Architecture } from "./components/Architecture";
import { Diagnostics } from "./components/Diagnostics";
import { Downloads } from "./components/Downloads";
import { Compatibility } from "./components/Compatibility";
import { FinalCta } from "./components/FinalCta";
import { Footer } from "./components/Footer";
import { LightboxProvider } from "./components/Lightbox";
import { useI18n } from "./i18n";

export default function App() {
  const { t } = useI18n();
  return (
    <LightboxProvider>
      <a className="skip-link" href="#telemetry">
        {t.a11y.skip}
      </a>
      <Nav />
      <main>
        <Hero />
        <Story />
        <ReadOnly />
        <Architecture />
        <Diagnostics />
        <Downloads />
        <Compatibility />
        <FinalCta />
      </main>
      <Footer />
    </LightboxProvider>
  );
}
