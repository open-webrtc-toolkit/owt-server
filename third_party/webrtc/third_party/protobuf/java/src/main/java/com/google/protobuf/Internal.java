// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// http://code.google.com/p/protobuf/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

package com.google.protobuf;

import java.io.UnsupportedEncodingException;

/**
 * The classes contained within are used internally by the Protocol Buffer
 * library and generated message implementations. They are public only because
 * those generated messages do not reside in the {@code protobuf} package.
 * Others should not use this class directly.
 *
 * @author kenton@google.com (Kenton Varda)
 */
public class Internal {
  /**
   * Helper called by generated code to construct default values for string
   * fields.
   * <p>
   * The protocol compiler does not actually contain a UTF-8 decoder -- it
   * just pushes UTF-8-encoded text around without touching it.  The one place
   * where this presents a problem is when generating Java string literals.
   * Unicode characters in the string literal would normally need to be encoded
   * using a Unicode escape sequence, which would require decoding them.
   * To get around this, protoc instead embeds the UTF-8 bytes into the
   * generated code and leaves it to the runtime library to decode them.
   * <p>
   * It gets worse, though.  If protoc just generated a byte array, like:
   *   new byte[] {0x12, 0x34, 0x56, 0x78}
   * Java actually generates *code* which allocates an array and then fills
   * in each value.  This is much less efficient than just embedding the bytes
   * directly into the bytecode.  To get around this, we need another
   * work-around.  String literals are embedded directly, so protoc actually
   * generates a string literal corresponding to the bytes.  The easiest way
   * to do this is to use the ISO-8859-1 character set, which corresponds to
   * the first 256 characters of the Unicode range.  Protoc can then use
   * good old CEscape to generate the string.
   * <p>
   * So we have a string literal which represents a set of bytes which
   * represents another string.  This function -- stringDefaultValue --
   * converts from the generated string to the string we actually want.  The
   * generated code calls this automatically.
   */
  public static String stringDefaultValue(String bytes) {
    try {
      return new String(bytes.getBytes("ISO-8859-1"), "UTF-8");
    } catch (UnsupportedEncodingException e) {
      // This should never happen since all JVMs are required to implement
      // both of the above character sets.
      throw new IllegalStateException(
          "Java VM does not support a standard character set.", e);
    }
  }

  /**
   * Helper called by generated code to construct default values for bytes
   * fields.
   * <p>
   * This is a lot like {@link #stringDefaultValue}, but for bytes fields.
   * In this case we only need the second of the two hacks -- allowing us to
   * embed raw bytes as a string literal with ISO-8859-1 encoding.
   */
  public static ByteString bytesDefaultValue(String bytes) {
    try {
      return ByteString.copyFrom(bytes.getBytes("ISO-8859-1"));
    } catch (UnsupportedEncodingException e) {
      // This should never happen since all JVMs are required to implement
      // ISO-8859-1.
      throw new IllegalStateException(
          "Java VM does not support a standard character set.", e);
    }
  }

  /**
   * Helper called by generated code to determine if a byte array is a valid
   * UTF-8 encoded string such that the original bytes can be converted to
   * a String object and then back to a byte array round tripping the bytes
   * without loss.
   * <p>
   * This is inspired by UTF_8.java in sun.nio.cs.
   *
   * @param byteString the string to check
   * @return whether the byte array is round trippable
   */
  public static boolean isValidUtf8(ByteString byteString) {
    int index = 0;
    int size = byteString.size();
    // To avoid the masking, we could change this to use bytes;
    // Then X > 0xC2 gets turned into X < -0xC2; X < 0x80
    // gets turned into X >= 0, etc.

    while (index < size) {
      int byte1 = byteString.byteAt(index++) & 0xFF;
      if (byte1 < 0x80) {
        // fast loop for single bytes
        continue;

        // we know from this point on that we have 2-4 byte forms
      } else if (byte1 < 0xC2 || byte1 > 0xF4) {
        // catch illegal first bytes: < C2 or > F4
        return false;
      }
      if (index >= size) {
        // fail if we run out of bytes
        return false;
      }
      int byte2 = byteString.byteAt(index++) & 0xFF;
      if (byte2 < 0x80 || byte2 > 0xBF) {
        // general trail-byte test
        return false;
      }
      if (byte1 <= 0xDF) {
        // two-byte form; general trail-byte test is sufficient
        continue;
      }

      // we know from this point on that we have 3 or 4 byte forms
      if (index >= size) {
        // fail if we run out of bytes
        return false;
      }
      int byte3 = byteString.byteAt(index++) & 0xFF;
      if (byte3 < 0x80 || byte3 > 0xBF) {
        // general trail-byte test
        return false;
      }
      if (byte1 <= 0xEF) {
        // three-byte form. Vastly more frequent than four-byte forms
        // The following has an extra test, but not worth restructuring
        if (byte1 == 0xE0 && byte2 < 0xA0 ||
            byte1 == 0xED && byte2 > 0x9F) {
          // check special cases of byte2
          return false;
        }

      } else {
        // four-byte form

        if (index >= size) {
          // fail if we run out of bytes
          return false;
        }
        int byte4 = byteString.byteAt(index++) & 0xFF;
        if (byte4 < 0x80 || byte4 > 0xBF) {
          // general trail-byte test
          return false;
        }
        // The following has an extra test, but not worth restructuring
        if (byte1 == 0xF0 && byte2 < 0x90 ||
            byte1 == 0xF4 && byte2 > 0x8F) {
          // check special cases of byte2
          return false;
        }
      }
    }
    return true;
  }

  /**
   * Interface for an enum value or value descriptor, to be used in FieldSet.
   * The lite library stores enum values directly in FieldSets but the full
   * library stores EnumValueDescriptors in order to better support reflection.
   */
  public interface EnumLite {
    int getNumber();
  }

  /**
   * Interface for an object which maps integers to {@link EnumLite}s.
   * {@link Descriptors.EnumDescriptor} implements this interface by mapping
   * numbers to {@link Descriptors.EnumValueDescriptor}s.  Additionally,
   * every generated enum type has a static method internalGetValueMap() which
   * returns an implementation of this type that maps numbers to enum values.
   */
  public interface EnumLiteMap<T extends EnumLite> {
    T findValueByNumber(int number);
  }
}
