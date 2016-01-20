/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

package tests.api.java.io;

import java.io.InvalidObjectException;

import dalvik.annotation.TestLevel;
import dalvik.annotation.TestTargetClass;
import dalvik.annotation.TestTargetNew;

@TestTargetClass(java.io.InvalidObjectException.class)
public class InvalidObjectExceptionTest extends junit.framework.TestCase {

    /**
     * @tests java.io.InvalidObjectException#InvalidObjectException(java.lang.String)
     */
    @TestTargetNew(
        level = TestLevel.COMPLETE,
        notes = "Verifies the InvalidObjectException(java.lang.String) constructor.",
        method = "InvalidObjectException",
        args = {java.lang.String.class}
    )
    public void test_ConstructorLjava_lang_String() {
        // Test for method java.io.InvalidObjectException(java.lang.String)
        try {
            if (true) throw new InvalidObjectException("This object is not valid.");
            fail("Exception not thrown.");
        } catch (InvalidObjectException e) {
            assertEquals("The exception message is not equal to the one " +
                         "passed to the constructor.",
                         "This object is not valid.", e.getMessage());
        }
    }
}
