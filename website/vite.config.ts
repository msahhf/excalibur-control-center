import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// Static product site: no backend, no API. `base: "./"` keeps the build portable
// (works from any sub-path, e.g. a future GitHub Pages directory) without
// committing to a hosting layout now.
export default defineConfig({
  base: "./",
  plugins: [react()],
  build: {
    outDir: "dist",
    target: "es2020",
    cssMinify: true,
  },
});
