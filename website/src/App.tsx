import { Nav } from "./components/Nav";
import { Hero } from "./components/Hero";
import { Scrolly } from "./components/Scrolly";
import { ReadOnly } from "./components/ReadOnly";
import { Architecture } from "./components/Architecture";
import { Diagnostics } from "./components/Diagnostics";
import { Downloads } from "./components/Downloads";
import { Compatibility } from "./components/Compatibility";
import { FinalCta } from "./components/FinalCta";
import { Footer } from "./components/Footer";
import { LightboxProvider } from "./components/Lightbox";

export default function App() {
  return (
    <LightboxProvider>
      <a className="skip-link" href="#telemetry">
        Skip to content
      </a>
      <Nav />
      <main>
        <Hero />
        <Scrolly />
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
