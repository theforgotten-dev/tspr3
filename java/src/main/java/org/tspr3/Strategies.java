package org.tspr3;
import java.util.Collections;
import java.util.EnumMap;
import java.util.Map;
final class Strategies {
    private static final Map<Tspr3.Algorithm,OrderingStrategy> REGISTRY;
    static {
        EnumMap<Tspr3.Algorithm,OrderingStrategy> entries=new EnumMap<>(Tspr3.Algorithm.class);
        entries.put(Tspr3.Algorithm.A,new PlainTriads());
        entries.put(Tspr3.Algorithm.B,new CompactTriads(false));
        entries.put(Tspr3.Algorithm.C,new CompactTriads(true));
        REGISTRY=Collections.unmodifiableMap(entries);
    }
    private Strategies() { }
    static OrderingStrategy get(Tspr3.Algorithm algorithm) {
        OrderingStrategy result=REGISTRY.get(algorithm);
        if(result==null) throw new IllegalArgumentException("Resolve AUTO before dispatch");
        return result;
    }
}
