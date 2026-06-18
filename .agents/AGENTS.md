## Core Troubleshooting Methodology

When debugging complex, persistent, or logical bugs, follow this systematic approach:

1. **Information Gathering & Contextualization**
   - Identify the exact symptoms. Differentiate between what is completely broken vs. what is partially working.
   - Locate the core components and map the high-level architecture of the feature.

2. **Trace the Data Flow (Source to Sink)**
   - Follow the data step-by-step from its input source to its final output/effect.
   - Identify every point where data is transformed, filtered, or evaluated.

3. **Hypothesis Generation & Elimination**
   - Identify obvious logic gaps, race conditions, or missing validations (e.g., missing type checks, duplicate event fires).
   - Apply fixes for these structural issues first, as they often mask the deeper root cause.

4. **"Listen to the Data" (Crucial Step)**
   - If the issue persists, heavily analyze the *exact* error values or outputs. 
   - Example: A return of `0` versus `-Infinity` versus `Null` tells entirely different stories. Use specific values to logically rule out entire classes of bugs (e.g., "If it returns 0, the memory allocation and pointer logic is fine; the math is wrong").

5. **Examine Fundamental Assumptions (The Deep Dive)**
   - When the code logic appears sound but the behavior is wrong, question the domains and environments.
   - **Common culprits to check:** Coordinate systems (handedness, axis alignment), Units of measurement (meters vs. game units), Time zones, 1-vs-0 indexing, Endianness, or synchronous vs. asynchronous execution orders.

6. **Mathematical/Logical Verification**
   - Do not guess the fix. Walk through a concrete, extreme edge-case example (e.g., "If X is exactly 0, what happens?").
   - Prove the mismatch or flaw mentally or on paper before modifying the code.

7. **Bidirectional Application of the Fix**
   - Once a fundamental flaw (like a coordinate mismatch) is found, ensure the fix is applied consistently across the entire lifecycle (e.g., fixing the Input -> System transform, and also the System -> Output transform).
